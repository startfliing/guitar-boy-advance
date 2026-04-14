#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
#include <cstdint>

// Trim leading and trailing whitespace
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// Convert string to valid C++ identifier
std::string toIdentifier(const std::string& str) {
    std::string result = trim(str);
    if (result.empty()) return "section";
    
    // Convert to lowercase and replace spaces/non-alphanumeric with underscores
    for (char& c : result) {
        if (std::isalnum(c)) {
            c = std::tolower(c);
        } else if (c != '_') {
            c = '_';
        }
    }
    
    // Ensure it doesn't start with a digit
    if (std::isdigit(result[0])) {
        result = "_" + result;
    }
    
    return result;
}

// Structure to hold note data
struct Note {
    uint64_t tick;
    uint32_t lane;
    uint32_t duration;
};

// Parse .chart file and extract notes from sections
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: chart_reader <input.chart> [output_dir]" << std::endl;
        return 1;
    }

    std::string chartFile = argv[1];
    std::string outputDir = (argc > 2) ? argv[2] : "build";
    
    // Create output directory if it doesn't exist
    std::filesystem::create_directories(outputDir);

    // Check if note.h exists in output directory, if not create it
    std::string noteHeaderFile = outputDir + "/note.h";
    if (!std::filesystem::exists(noteHeaderFile)) {
        std::ofstream noteHeaderOut(noteHeaderFile);
        if (!noteHeaderOut.is_open()) {
            std::cerr << "Error: Cannot write to file " << noteHeaderFile << std::endl;
            return 1;
        }
        
        noteHeaderOut << "#ifndef __NOTE__\n";
        noteHeaderOut << "#define __NOTE__\n\n";
        noteHeaderOut << "#include \"tonc.h\"\n\n";
        noteHeaderOut << "struct Note {\n";
        noteHeaderOut << "    u64 tick;\n";
        noteHeaderOut << "    u32 lane;\n";
        noteHeaderOut << "    u32 duration;\n";
        noteHeaderOut << "};\n\n";
        noteHeaderOut << "#endif\n";
        
        noteHeaderOut.close();
    }

    // Read chart file
    std::ifstream infile(chartFile);
    if (!infile.is_open()) {
        std::cerr << "Error: Cannot open file " << chartFile << std::endl;
        return 1;
    }

    // Map of section names to their notes
    std::map<std::string, std::vector<Note>> sections;
    std::string currentSection;
    std::string line;
    uint32_t firstBPM = 0;  // Store the first BPM value from SyncTrack
    uint32_t resolution = 0;  // Store the resolution value from Song section
    bool bpmFound = false;
    bool resolutionFound = false;
    bool inSyncTrack = false;
    bool inSongSection = false;

    // Parse the chart file
    while (std::getline(infile, line)) {
        // Skip UTF-8 BOM if present at the beginning of the file
        if (line.size() >= 3 && (unsigned char)line[0] == 0xEF && 
            (unsigned char)line[1] == 0xBB && (unsigned char)line[2] == 0xBF) {
            line = line.substr(3);
        }
        
        line = trim(line);

        // Skip empty lines
        if (line.empty()) continue;
        
        // Check for section headers
        if (line[0] == '[' && line[line.length() - 1] == ']') {

            std::string sectionName = line.substr(1, line.length() - 2);
            
            // Handle SyncTrack section
            if (sectionName == "SyncTrack") {
                inSyncTrack = true;
                inSongSection = false;
                currentSection = "";
            } else if (sectionName == "Song") {
                inSyncTrack = false;
                inSongSection = true;
                currentSection = "";
            } else if (sectionName == "Events") {
                inSyncTrack = false;
                inSongSection = false;
                currentSection = "";
            } else {
                inSyncTrack = false;
                inSongSection = false;
                currentSection = sectionName;
                sections[currentSection] = std::vector<Note>();
            }
            continue;
        }
        
        // Skip opening brace
        if (line == "{") continue;
        
        // Stop at closing brace
        if (line == "}") {
            if (inSyncTrack) inSyncTrack = false;
            if (inSongSection) inSongSection = false;
            currentSection = "";
            continue;
        }
        
        // Process SyncTrack entries to find first BPM
        if (inSyncTrack && !bpmFound) {
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string rest = trim(line.substr(eqPos + 1));
                if (!rest.empty() && rest[0] == 'B') {
                    // Extract BPM value (format: "B 175000")
                    std::istringstream iss(rest.substr(1));
                    uint32_t bpmValue;
                    if (iss >> bpmValue) {
                        firstBPM = bpmValue;  // Convert from thousandths to actual BPM
                        bpmFound = true;
                    }
                }
            }
        }
        
        // Process Song section entries to find resolution
        if (inSongSection && !resolutionFound) {
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = trim(line.substr(0, eqPos));
                std::string value = trim(line.substr(eqPos + 1));
                
                if (key == "Resolution") {
                    std::istringstream iss(value);
                    uint32_t resolutionValue;
                    if (iss >> resolutionValue) {
                        resolution = resolutionValue;
                        resolutionFound = true;
                    }
                }
            }
        }
        
        // Process note lines within a section
        if (!currentSection.empty()) {
            // Parse line format: "tick = N lane duration" or "tick = N lane 0" or variation
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string tickStr = trim(line.substr(0, eqPos));
                std::string rest = trim(line.substr(eqPos + 1));
                
                // Only process lines starting with 'N'
                if (!rest.empty() && rest[0] == 'N') {
                    // Extract the three numeric values
                    std::istringstream iss(rest.substr(1)); // Skip the 'N'
                    uint64_t tick;
                    uint32_t lane, duration;
                    
                    if (std::istringstream(tickStr) >> tick && iss >> lane >> duration) {
                        //Not sure why this works, but it works: 59.73 * 59.73 * 1000 = 3567673
                        //(60 seconds/minute) * (59.73 Hz) * (1000 for BPM conversion) = 3583800
                        //(60 seconds/minute) * (60 Hz) * (1000 for BPM conversion) = 3600000
                        uint64_t tickValue = ((double)tick/resolution)*3567673;
                        sections[currentSection].push_back({tickValue, lane, duration});
                    }
                }
            }
        }
    }

    infile.close();

    if (sections.empty()) {
        std::cerr << "Error: No valid sections found in chart file" << std::endl;
        return 1;
    }

    std::string filenameStem = std::filesystem::path(chartFile).stem().string();

    // Generate single assembly file for all sections
    std::string asmFile = outputDir + "/" + filenameStem + ".s";
    std::ofstream asmout(asmFile);
    if (!asmout.is_open()) {
        std::cerr << "Error: Cannot write to file " << asmFile << std::endl;
        return 1;
    }

    // Write main header comment block
    asmout << "@{{BLOCK(" << filenameStem << ")\n\n";
    asmout << "@=======================================================================\n";
    asmout << "@\n";
    asmout << "@\t" << filenameStem << " - Chart note data\n";
    asmout << "@\n";
    asmout << "@\tGenerated from: " << chartFile << "\n";
    asmout << "@\tSections: " << sections.size() << "\n";
    asmout << "@\n";
    asmout << "@=======================================================================\n\n";

    asmout << ".section .rodata\n";

    // Write all sections to the same file
    for (const auto& [sectionName, notes] : sections) {
        if (notes.empty()) continue;

        std::string identifier = toIdentifier(sectionName);
        std::string dataSymbolBase = filenameStem + "_" + identifier;
        std::string dataSymbol = dataSymbolBase + "_data";
        std::string countSymbol = dataSymbolBase + "_count";

        // Write section comment header
        asmout << "@\n";
        asmout << "@\tSection: " << sectionName << " (" << notes.size() << " notes)\n";
        asmout << "@\n";

        // Calculate data size: 8 bytes (tick) + 4 bytes (lane) + 4 bytes (duration) = 16 bytes per note
        size_t recordSize = 16; // 8 + 4 + 4
        size_t totalDataSize = recordSize * notes.size();

        asmout << ".align 2\n";
        asmout << ".global " << dataSymbol << "\t\t@ " << totalDataSize << " bytes\n";
        asmout << dataSymbol << ":\n";

        // Write note data
        for (const auto& note : notes) {
            // tick as 8-byte word
            asmout << "\t.8byte " << note.tick << "\n";
            // lane as 4-byte word
            asmout << "\t.4byte " << note.lane << "\n";
            // duration as 4-byte word
            asmout << "\t.4byte " << note.duration << "\n";
        }

        asmout << ".size " << dataSymbol << ", . - " << dataSymbol << "\n\n";

        // Count symbol
        asmout << ".global " << countSymbol << "\t\t@ 4 bytes\n";
        asmout << countSymbol << ":\n";
        asmout << "\t.4byte " << notes.size() << "\n";
        asmout << ".size " << countSymbol << ", 4\n\n";
    }

    asmout << "@}}BLOCK(" << filenameStem << ")\n";
    asmout.close();

    // Generate single header file with struct definition and all extern declarations
    std::string headerFile = outputDir + "/" + filenameStem + ".h";
    std::string guardName = "DATA_" + filenameStem;
    std::transform(guardName.begin(), guardName.end(), guardName.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    for (char& c : guardName) {
        if (!std::isalnum(c)) c = '_';
    }

    std::ofstream outfile(headerFile);
    if (!outfile.is_open()) {
        std::cerr << "Error: Cannot write to file " << headerFile << std::endl;
        return 1;
    }

    // Header guard
    outfile << "#ifndef " << guardName << "_H\n";
    outfile << "#define " << guardName << "_H\n\n";
    outfile << "#include <cstdint>\n\n";
    outfile << "#include \"note.h\"\n\n";

    // Add BPM constant if found
    if (bpmFound && firstBPM > 0) {
        outfile << "// BPM from SyncTrack\n";
        outfile << "const u64 " << filenameStem << "_BPM = " << firstBPM << ";\n\n";
    }

    // Extern declarations for all sections
    for (const auto& [sectionName, notes] : sections) {
        if (notes.empty()) continue;

        std::string identifier = toIdentifier(sectionName);
        std::string dataSymbolBase = filenameStem + "_" + identifier;
        std::string dataSymbol = dataSymbolBase + "_data";
        std::string countSymbol = dataSymbolBase + "_count";

        outfile << "// Section: " << sectionName << "\n";
        outfile << "extern const struct Note " << dataSymbol << "[];\n";
        outfile << "#define " << countSymbol << " " << notes.size() << "\n\n";
    }

    outfile << "#endif // " << guardName << "_H\n";

    outfile.close();

    //std::cout << "Successfully generated " << sections.size() << " section file(s) from " << chartFile << std::endl;

    return 0;
}

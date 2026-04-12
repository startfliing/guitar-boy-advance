#ifndef __NOTE_MANAGER__
#define __NOTE_MANAGER__

#include "tonc.h"
#include "terminal.hpp"
#include "note.h"  // For struct Note definition

struct noteSprite{
    VECTOR pos;
    u8 pal;
};

template <u32 N>
class NoteManager{
    public:
        NoteManager(const Note* song_notes, u32 song_bpm) {
            this->song_notes = song_notes;
            this->note_ct = N;
            this->song_bpm = song_bpm; //minutes to seconds to frames
            this->current_tick = 0;
            this->current_frame = 0;
            this->current_note_index = 0;
            this->prev_note_lanes = 0;
            prepSprites();
        }

        int update() {
            if(current_tick >= song_notes[current_note_index].tick) {

                //iterate through all notes that should be spawned at this tick
                while(current_tick >= song_notes[current_note_index].tick){
                    Terminal::log("Note %% hit box, z: %%", current_note_index, note_sprites[current_note_index].pos.z);
                    // Move to the next note in the song
                    current_note_index++;
                }

            }
            current_tick += song_bpm; // Increment tick based on BPM (assuming update is called every frame at 60 FPS)
            return current_note_index;
        };

        noteSprite* getNoteSprites() {
            return note_sprites;
        }

    private:
        void prepSprites(){
            for(u32 i = 0; i < N; i++){
                VECTOR pos;
                pos.y = 0;
                switch(song_notes[i].lane){
                    case 0:
                        note_sprites[i].pal = 1;
                        pos.x = 0x0E300;
                        break;
                    case 1:
                        note_sprites[i].pal = 2;
                        pos.x = 0x0F400;
                        break;
                    case 2:
                        note_sprites[i].pal = 3;
                        pos.x = 0x10400;
                        break;
                    case 3:
                        note_sprites[i].pal = 4;
                        pos.x = 0x11600;
                        break;
                }
                s64 temp_tick = song_notes[i].tick * -1;
                pos.z = (((256 * temp_tick / 875) - 640000)>>4)-25000; //this is a placeholder, will need to be converted to actual z position based on tempo and speed of highway
                note_sprites[i].pos = pos;
            }
        }
        
        const Note* song_notes;
        noteSprite note_sprites[N];
        u32 song_bpm;
        u32 note_ct;
        u64 current_tick;
        u32 current_frame;
        u32 current_note_index;

        u16 prev_note_lanes;
        u16 next_note_lanes;
};

#endif // __NOTE_MANAGER_HPP__

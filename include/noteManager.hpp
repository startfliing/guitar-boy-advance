#ifndef __NOTE_MANAGER__
#define __NOTE_MANAGER__

#include "tonc.h"
#include "terminal.hpp"
#include "textDisplay.hpp"
#include "note.h"  // For struct Note definition

struct noteSprite{
    VECTOR pos;
    u8 pal;
};

#define ENABLED true

template <u32 N>
class NoteManager{
    public:
        NoteManager(const Note* song_notes, u32 song_bpm) {
            this->song_notes = song_notes;
            this->streak_text = new textDisplay(4, 4, 32);
            this->percentage_text = new textDisplay(236, 4, 48, false);
            this->note_ct = N;
            this->song_bpm = song_bpm; //minutes to seconds to frames
            this->current_tick = 0;
            this->current_frame = 0;
            this->current_note_index = 0;
            this->curr_note_lanes = 0;
            this->note_check = 0;
            this->notes_hit = 0;
            this->current_streak = 0;
            prepSprites();
        }

        void init(){
            if(ENABLED) percentage_text->update(0, N);
            if(ENABLED) streak_text->update(0);
        }

        int update() {
            //right before hit window, set up what lanes should we expect the player to hit
            if(current_tick >= song_notes[current_note_index].tick - (song_bpm * 10)){ 
                u16 note_lanes = 0;
                note_check = current_note_index;
                //iterate through all notes that should be spawned at this tick
                while(current_tick >= song_notes[current_note_index].tick - (song_bpm * 10)){
                    //Terminal::log("UPPER Range of note %%", current_note_index);
                    // Move to the next note in the song
                    switch(song_notes[current_note_index].lane){
                        case 0:
                            note_lanes |= KEY_L;
                            break;
                        case 1:
                            note_lanes |= KEY_B;
                            break;
                        case 2:
                            note_lanes |= KEY_A;
                            break;
                        case 3:
                            note_lanes |= KEY_R;
                            break;
                    }
                    current_note_index++;
                }
                curr_note_lanes = note_lanes;
            }
            current_tick += song_bpm; // Increment tick based on BPM (assuming update is called every frame at 60 FPS)
            return current_note_index;
        };

        noteSprite* getNoteSprites() {
            return note_sprites;
        }

        int checkHit(u32 keys_hit){
            //if within hit window of current note, check if correct button is pressed
            
            if(current_tick >= song_notes[note_check].tick - (song_bpm * 3) && current_tick <= song_notes[note_check].tick + (song_bpm * 5)){

                if(keys_hit == curr_note_lanes){
                    if(ENABLED) percentage_text->update(++notes_hit, N);
                    if(ENABLED) streak_text->update(++current_streak);
                    return note_check; // Note hit successfully
                }

            }
            
            current_streak = 0;
            if(ENABLED) streak_text->update(current_streak);
            return -1;
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
                pos.z = (((256 * temp_tick / 871) - 640000)>>4)-25000; //this is a placeholder, will need to be converted to actual z position based on tempo and speed of highway
                note_sprites[i].pos = pos;
            }
        }
        
        const Note* song_notes;
        noteSprite note_sprites[N];
        textDisplay* streak_text;
        textDisplay* percentage_text;
        u32 song_bpm;
        u32 note_ct;
        u64 current_tick;
        u32 current_frame;
        u32 current_note_index;
        u32 current_streak;
        u32 notes_hit;

        u16 curr_note_lanes;
        u32 note_check;
};

#endif // __NOTE_MANAGER_HPP__

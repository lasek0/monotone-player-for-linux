#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <string.h>

#define MAX_FILE_SIZE 0xffff

#ifndef CLOCK_TICK_RATE
#define CLOCK_TICK_RATE 1193180
#endif


struct MT_Note {
        union {
                struct {
                        uint16_t effect_data:6;
                        uint16_t effect_id:3;
                        uint16_t note:7;
                };
                struct {
                        uint16_t effect_data_x:3;
                        uint16_t effect_data_y:3;
                };
                uint16_t raw;
        };
};

struct MT_Track {
        int enabled;
        int duration_id;
        int duration_slide;
};

struct MT_Song {
        int tempo_base;
        int tempo;
        int tracks;
};

#pragma pack(push)
#pragma pack(1)
struct MT_Header {
        uint8_t mag_str_len; // 8
        char    mag_str[8];
        uint8_t title_str_len; // 40
        char    title_str[40];
        uint8_t comment_str_len; // 40
        char    comment_str[40];
        uint8_t version;
        uint8_t total_patterns;
        uint8_t total_tracks;
        uint8_t cell_size; // these three along with 64 rows per pattern tell us how much to load
        uint8_t pattern_order[255]; // {FF = end of song so fill with FFs}
        uint8_t note_data_size;
        struct MT_Note notes_data[1];//1 just to compiler think it is array but acutalli there is
                                     // total_patterns * total_tracks number of MT_Note's
};
#pragma pack(pop)

const char note_labels[128][4] = {
        "---",
        "A-0", "A#0", "B-0",
        "C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",
        "C-2", "C#2", "D-2", "D#2", "E-2", "F-2", "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",
        "C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3",
        "C-4", "C#4", "D-4", "D#4", "E-4", "F-4", "F#4", "G-4", "G#4", "A-4", "A#4", "B-4",
        "C-5", "C#5", "D-5", "D#5", "E-5", "F-5", "F#5", "G-5", "G#5", "A-5", "A#5", "B-5",
        "C-6", "C#6", "D-6", "D#6", "E-6", "F-6", "F#6", "G-6", "G#6", "A-6", "A#6", "B-6",
        "C-7", "C#7", "D-7", "D#7", "E-7", "F-7", "F#7", "G-7", "G#7", "A-7", "A#7", "B-7",
        "C-8", "C#8", "D-8", "D#8", "E-8", "F-8", "F#8", "G-8", "G#8", "A-8", "A#8", "B-8",
        "C-9",
        "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???",
        "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???",
        "???", "???",
        "OFF"
};


//TODO: convert to nearest int!
//source: https://pages.mtu.edu/~suits/notefreqs.html
const float note_freq_table[] = {
/*A0            */27.50,
/* A#0/Bb0      */29.14,
/*B0            */30.87,
/*C1            */32.70,
/* C#1/Db1      */34.65,
/*D1            */36.71,
/* D#1/Eb1      */38.89,
/*E1            */41.20,
/*F1            */43.65,
/* F#1/Gb1      */46.25,
/*G1            */49.00,
/* G#1/Ab1      */51.91,
/*A1            */55.00,
/* A#1/Bb1      */58.27,
/*B1            */61.74,
/*C2            */65.41,
/* C#2/Db2      */69.30,
/*D2            */73.42,
/* D#2/Eb2      */77.78,
/*E2            */82.41,
/*F2            */87.31,
/* F#2/Gb2      */92.50,
/*G2            */98.00,
/* G#2/Ab2      */103.83,
/*A2            */110.00,
/* A#2/Bb2      */116.54,
/*B2            */123.47,
/*C3            */130.81,
/* C#3/Db3      */138.59,
/*D3            */146.83,
/* D#3/Eb3      */155.56,
/*E3            */164.81,
/*F3            */174.61,
/* F#3/Gb3      */185.00,
/*G3            */196.00,
/* G#3/Ab3      */207.65,
/*A3            */220.00,
/* A#3/Bb3      */233.08,
/*B3            */246.94,
/*C4            */261.63,
/* C#4/Db4      */277.18,
/*D4            */293.66,
/* D#4/Eb4      */311.13,
/*E4            */329.63,
/*F4            */349.23,
/* F#4/Gb4      */369.99,
/*G4            */392.00,
/* G#4/Ab4      */415.30,
/*A4            */440.00,
/* A#4/Bb4      */466.16,
/*B4            */493.88,
/*C5            */523.25,
/* C#5/Db5      */554.37,
/*D5            */587.33,
/* D#5/Eb5      */622.25,
/*E5            */659.25,
/*F5            */698.46,
/* F#5/Gb5      */739.99,
/*G5            */783.99,
/* G#5/Ab5      */830.61,
/*A5            */880.00,
/* A#5/Bb5      */932.33,
/*B5            */987.77,
/*C6            */1046.50,
/* C#6/Db6      */1108.73,
/*D6            */1174.66,
/* D#6/Eb6      */1244.51,
/*E6            */1318.51,
/*F6            */1396.91,
/* F#6/Gb6      */1479.98,
/*G6            */1567.98,
/* G#6/Ab6      */1661.22,
/*A6            */1760.00,
/* A#6/Bb6      */1864.66,
/*B6            */1975.53,
/*C7            */2093.00,
/* C#7/Db7      */2217.46,
/*D7            */2349.32,
/* D#7/Eb7      */2489.02,
/*E7            */2637.02,
/*F7            */2793.83,
/* F#7/Gb7      */2959.96,
/*G7            */3135.96,
/* G#7/Ab7      */3322.44,
/*A7            */3520.00,
/* A#7/Bb7      */3729.31,
/*B7            */3951.07,
/*C8            */4186.01,
/* C#8/Db8      */4434.92,
/*D8            */4698.63,
/* D#8/Eb8      */4978.03,
/*E8            */5274.04,
/*F8            */5587.65,
/* F#8/Gb8      */5919.91,
/*G8            */6271.93,
/* G#8/Ab8      */6644.88,
/*A8            */7040.00,
/* A#8/Bb8      */7458.62,
/*B8            */7902.13
};


int note_dutation_table[sizeof(note_freq_table)/sizeof(float)];

const char effect_label[8] = "01234BDF";



int SetupTrack(struct MT_Song* s, struct MT_Track* t, struct MT_Note* n) {
        //TODO: check NULL-s!!!

        if(n->note > 0 && n->note < 127){
                t->enabled = 1;
                t->duration_id = n->note-1;
                t->duration_slide = 0;
        }else if(n->note == 127){
                t->enabled = 0;
        }

        //TODO:
        switch(n->effect_id){
                case /*0*/0:
                        break;
                case /*1*/1: /*slide up speed 1xx*/
                        t->duration_slide -= n->effect_data; // frequency goes up
                        break;
                case /*2*/2: /*slide down speed 2xx*/
                        t->duration_slide += n->effect_data; // frequency foes down
                        break;
                case /*3*/3: /*tone portamento up/down speed 3xx */
                        break;
                case /*4*/4: /*vibrato 4xy s-speed, y-depth*/
                        break;
                case /*B*/5: /*jump song position Bxx xx-song position*/
                        break;
                case /*D*/6: /*pattern break: Dxx break position in next pattern*/
                        return 1;// break current pattern
                        break;
                case /*F*/7: /*set tempo/speed xx(00-1f) for speed, xx(20-7f) for tempo*/
                        s->tempo = /*s->tempo_base + */n->effect_data;
                        break;
        }
        return 0;
}

void PlayTrack(const int console_fd, struct MT_Song* s, struct MT_Track* t) {
        //TODO: check NULL-s!!!
        if(t->enabled)
                ioctl(console_fd, KIOCSOUND, note_dutation_table[t->duration_id] + t->duration_slide);
        else
                ioctl(console_fd, KIOCSOUND, 0);

        usleep(8333 + (8333/s->tracks*s->tempo));
}


int main(int argc, char* argv[]) {

        //pre calculate note_dutation_table
        for(int f=0; f<sizeof(note_freq_table)/sizeof(float); f++){
                note_dutation_table[f] = CLOCK_TICK_RATE/note_freq_table[f];
        }

        if(argc != 2){
                printf("usage: monbeeper <mon_file_path>\n");
                return 0;
        }

        FILE *f = fopen(argv[1], "rb");
        if(f == NULL){
                printf("ERROR: can not open file '%s'.\n", argv[1]);
                return -1;
        }

        fseek(f, 0, SEEK_END);
        uint32_t fsz = ftell(f);
        fseek(f, 0, SEEK_SET);

        if(fsz == 0 || fsz > MAX_FILE_SIZE){
                fclose(f);
                printf("ERROR: file '%s' size is invalid.\n", argv[1]);
                return -2;
        }

        uint8_t *d = (uint8_t*)malloc(fsz);
        if(d == NULL){
                fclose(f);
                printf("ERROR: can not allocate %d bytes of memory\n", fsz);
                return -3;
        }

        if(fsz != fread(d, 1, fsz, f)){
                free(d);
                fclose(f);
                printf("ERROR: can not read %d bytes of memory from file '%s'\n", fsz, argv[1]);
                return -4;
        }
        fclose(f);


        struct MT_Header *h = (struct MT_Header*)d;
        printf("HEADER: \n");
        printf("mag_str_len     : %d\n", h->mag_str_len);
        printf("mag_str         : %s\n", h->mag_str);
        printf("title_str_len   : %d\n", h->title_str_len);
        printf("title_str       : %s\n", h->title_str);
        printf("comment_str_len : %d\n", h->comment_str_len);
        printf("comment_str     : %s\n", h->comment_str);
        printf("version         : %d\n", h->version);
        printf("total_patterns  : %d\n", h->total_patterns);
        printf("total_tracks    : %d\n", h->total_tracks);
        printf("cell_size       : %d\n", h->cell_size);
        printf("pattern_order   : \n\t");
        for(uint8_t i=0; i<h->total_patterns; i++){
                printf("%.2x ", h->pattern_order[i]);
        }
        printf("\n");

        //sanity chceck
        if(h->total_tracks > 10){
                h->total_tracks = 10;
        }


        struct MT_Song song;
        memset(&song, 0, sizeof(struct MT_Song));
        song.tempo = h->total_tracks;
        song.tracks = h->total_tracks;


        //allocate tracks
        const int tr_size = sizeof(struct MT_Track) * h->total_tracks;
        struct MT_Track* tr = (struct MT_Track*)malloc(tr_size);
        if(tr == NULL){
                free(d);
                printf("ERROR: can not open allocate %d bytes of memory\n", tr_size);
                return -6;
        }
        memset(tr, 0, tr_size);

        // open beeper console
        const int console_fd  = open("/dev/tty1", O_WRONLY);
        if(console_fd < 0){
                free(tr);
                free(d);
                printf("ERROR: can not open pc speeker device\n");
                return -5;
        }


        // play

        for(uint8_t i=0; i<h->total_patterns; i++){
                uint8_t cur_pattern = h->pattern_order[i];

                memset(tr, 0, tr_size);

                //calc ptr to patterm data
                for(uint8_t j=0; j<0x40; j++){
                        printf("%.2x | %.2x ", cur_pattern, j);
                        for(uint8_t k=0; k<h->total_tracks; k++){
                                struct MT_Note *note = &h->notes_data[
                                        cur_pattern * 0x40 * h->total_tracks +
                                        j * h->total_tracks + k ];

                                printf("| %3s %c%.2x ",
                                        note_labels[note->note],
                                        effect_label[note->effect_id],
                                        note->effect_data
                                );

                                if(SetupTrack(&song, &tr[k], note)){
                                        j = 0x40;
                                        break;
                                }
                                PlayTrack(console_fd, &song, &tr[k]);

                        }
                        printf("|\n");
                }

        }

        ioctl(console_fd, KIOCSOUND, 0);
        close(console_fd);

        free(tr);
        free(d);

        return 0;
}


#include "hwif.c"

enum CARDTYPES
{
    CARD_NOCARD,
    CARD_ATK1,
    CARD_ATK2,
    CARD_ATK3,
    CARD_DEF1,
    CARD_DEF2,
    CARD_DEF3,
    CARD_FOCUS,
    CARD_LEAP,
    CARD_ATK1DEF1,
    CARD_ATK2DEF1,
    CARD_ATK1DEF2,
    CARD_TBD,
    CARD_IDLE,
    CARD_FATIGUE,
    CARD_HURT,
    CARD_BACK
};

typedef struct Card_
{
	unsigned char mAttack;
	unsigned char mDefend;
	unsigned char mFlags;
	char *mName;
} Card;

enum CARDFLAGS
{
    CARDFLAG_LEAP       = 0b0010000,
    CARDFLAG_FOCUS      = 0b0001000,
    CARDFLAG_IDLE       = 0b0000100,
    CARDFLAG_FATIGUE    = 0b0000010,
    CARDFLAG_PERMANENT  = 0b0000001
};

#define COLOR(BLINK, BRIGHT, PAPER, INK) (((BLINK) << 7) | ((BRIGHT) << 6) | ((PAPER) << 3) | (INK))
#define TRIGGER(x) ((key_wasdown & (x)) && !(key_isdown & (x)))

extern const unsigned char littlesin[];
extern const unsigned char fatigue_for_cards[];
extern const Card gCardTypes[];
extern const unsigned char backtile[];
extern const unsigned char artassets[];
extern const unsigned short yofs[];
extern const unsigned char propfont_data[];
extern const unsigned char propfont_width[];

extern unsigned char *data_ptr;
extern unsigned char *screen_ptr;
extern unsigned char port254tonebit;
extern unsigned char y8;
extern unsigned char key_isdown;
extern unsigned char key_wasdown;
extern unsigned char input_mode;
extern unsigned char player_money;
extern unsigned char player_hurt;

enum SIMPLEKEYS
{
    KEY_LEFT = 1 << 0,
    KEY_RIGHT = 1 << 1,
    KEY_UP = 1 << 2,
    KEY_DOWN = 1 << 3,
    KEY_FIRE = 1 << 4
};

enum INPUTMODE
{
    INPUT_WASD,
    INPUT_QAOP,
    INPUT_CURSOR,
    INPUT_KEMPSTON,
    INPUT_SINCLAIR
};

extern void playfx(unsigned short fx) __z88dk_fastcall;  
extern void colorbox(unsigned char x, unsigned char y, unsigned char w, unsigned char h, unsigned char c);
extern void fillback();
extern void drawcard(unsigned char cardno,  unsigned char x, unsigned char y, unsigned char c);
extern void drawmug(unsigned char mugno,  unsigned char x, unsigned char y);
extern void drawicon(unsigned char iconno,  unsigned char x, unsigned char y);
extern void cleartextbox(unsigned char x, unsigned char y, unsigned char w, unsigned char h);
extern void drawtextbox(unsigned char x, unsigned char y, unsigned char w, unsigned char h);
extern void cleartextbox_color(unsigned char x, unsigned char y, unsigned char w, unsigned char h, unsigned char c);
extern void drawtextbox_color(unsigned char x, unsigned char y, unsigned char w, unsigned char h, unsigned char c);
extern void drawstringz(unsigned char *aS, unsigned char aX, unsigned char aY);

extern void drawmoney(unsigned char x, unsigned char y, unsigned char v);
extern void drawcost(unsigned char x, unsigned char y, unsigned char v);
extern void int2str(unsigned char val, char s[4]);
extern void strcat(char *tgt, char *src);
extern void scan_input();
extern void ingame();
extern void cardfx();
extern void tour();
extern void shop();
extern void heal();

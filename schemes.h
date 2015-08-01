//-----------------------------------------------------------------------------
// Prices' Pulse Viewer
// Color schemes definition file
// Menu structure definition file
// Ural-Relcom
// Sergey Gershtein
// Borland C++ Ver. 4.5
// Platform: DOS Standard; memory model: large (far code, far data, 64K static)
// Started: 5-Oct-95
//-----------------------------------------------------------------------------

#include <conio.h> // we need color contants

// color scheme constants
#define scMono 0
#define scColor 1
#define scLaptop 2
#define scUser 3
#define scCheck 255

#define scMaxScheme 3  // maximum valid number

// attributes constans
enum e_attrs {attrBkgr, // background attribute
            attrStatus, // status bar attribute
            attrWorking, // "WORKING" sign on status bar attribute
            attrWindow, // Window attribute
            attrMenu,   // Menu regular attribute
            attrMenuSel,   // Selected menu entry
            attrMenuChar,  // Slector character in menu entry
            attrMenuDisab, // Disabled menu entry
            attrMenuSelDisab, // Selected disabled menu entry
            attrButton,       // Button
            attrButtonFocus,  // Focused button
            attrButtonSelChar, // Button Selection char
            attrDefButton,    // Default Button
            attrMesBox,       // Message Box
            attrRED,          // text color for error message text
            attrGREEN,        // text color for ok status text
            attrScrollBar,    // scrollbar
            attrSBHandle,     // scroll bar handle
            attrListWin,      // list window attribute
            attrListWinSel,   // list window selected attribute
            attrListWinCur,   // list window currecnt line attribute
            attrListWinCurSel,// list window cur line selected attribute
            attrAddrInfo,     // full address and phones info window
            attrAddrData,     // full address and phones data in the window
            attrInputBox,     // input box attribute
            attrPercent1,     // percentage bar for "done"
            attrPercent2,     // percentage bar for "remaining"
            attrCurRates,     // Currency Rates window
            attrAddrInfoF,    // full address and phones info window (firms)
            attrAddrDataF,    // full addr and phone data in the window (firms)
				attrYELLOW,       // text color for highlighted text
				attrUserName,		// username attribute

				attr__};     // not for use

#define attrMax (attr__-1)

// attributes for mono displays
#define mBLACK 0x0
#define mUNDERLINE 0x1
#define mNORMAL 0x7
#define mBRIGHT 0x8
#define mREVERSE 0x70
#define mBLINK BLINK

// the attributes itself  { mono, color, laptop, user }
#define ATTRIBUTES   /*  the attributes of different screen elements  */ \
 {mNORMAL          , (CYAN<<4)+BLACK, 0, 0}        /* attrBkgr          */\
,{mREVERSE         , (LIGHTGRAY<<4)+BLACK, 0, 0}       /* attrStatus        */\
,{mREVERSE+mBLINK  , (BLACK<<4)+LIGHTRED+BLINK, 0, 0}  /* attrWorking       */\
,{mREVERSE         , (CYAN<<4)+WHITE,0,0}              /* attrWindow        */\
,{mREVERSE         , (LIGHTGRAY<<4)+BLACK, 0, 0}       /* attrMenu          */\
,{mNORMAL          , (BLACK<<4)+WHITE, 0, 0}           /* attrMenuSel       */\
,{mREVERSE+mBRIGHT , (LIGHTGRAY<<4)+YELLOW, 0, 0}      /* attrMenuChar      */\
,{mREVERSE+mNORMAL , (LIGHTGRAY<<4)+DARKGRAY, 0, 0}    /* attrMenuDisab     */\
,{mBLACK           , (BLACK<<4)+DARKGRAY, 0, 0}        /* attrMenuSelDisab  */\
,{mNORMAL          , (GREEN<<4)+BLACK, 0, 0}           /* attrButton        */\
,{mNORMAL+mUNDERLINE,(GREEN<<4)+WHITE, 0, 0}           /* attrButtonFocus   */\
,{mNORMAL+mBRIGHT  , (GREEN<<4)+YELLOW, 0, 0}          /* attrButtonSelChar */\
,{mNORMAL+mBRIGHT  , (GREEN<<4)+LIGHTCYAN, 0, 0}       /* attrDefButton     */\
,{mREVERSE         , (MAGENTA  <<4)+WHITE, 0, 0}       /* attrMesBox        */\
,{mREVERSE         , LIGHTRED          , 0, 0}         /* attrRED           */\
,{mREVERSE         , LIGHTGREEN        , 0, 0}         /* attrGREEN         */\
,{mREVERSE         , (LIGHTGRAY<<4)+BLACK, 0, 0}       /* attrScrollBar     */\
,{mNORMAL          , LIGHTGRAY           , 0, 0}       /* attrSBHandle      */\
,{mREVERSE         , ( BLUE<<4)+LIGHTGRAY, 0, 0}       /* attrListWin       */\
,{mREVERSE+mBRIGHT ,(     BLUE<<4)+YELLOW,0, 0}      /* attrListWinSel    */\
,{mNORMAL          , (LIGHTGRAY<<4)+BLACK, 0, 0}       /* attrListWinCur    */\
,{mNORMAL+mBRIGHT  ,(LIGHTGRAY<<4)+YELLOW,0, 0}       /* attrListWinCurSel */\
,{mREVERSE+mBRIGHT , (CYAN     <<4)+WHITE ,0, 0}       /* attrAddrInfo      */\
,{mREVERSE         , (CYAN <<4)+BLACK     ,0, 0}       /* attrAddrData      */\
,{mNORMAL          , (BLUE     <<4)+WHITE ,0, 0}       /* attrInputBox      */\
,{mNORMAL          , (BLUE     <<4)+WHITE ,0, 0}       /* attrPercent1      */\
,{mREVERSE         , (LIGHTGRAY<<4)+BLUE  ,0, 0}       /* attrPercent2      */\
,{mREVERSE         , (BLUE     <<4)+WHITE ,0, 0}       /* attrCurRates      */\
,{mREVERSE+mBRIGHT , (BLUE     <<4)+WHITE ,0, 0}       /* attrAddrInfoF     */\
,{mREVERSE         , (BLUE <<4)+LIGHTGRAY ,0, 0}       /* attrAddrDataF     */\
,{mREVERSE         , YELLOW               ,0, 0}       /* attrAddrDataF     */\
,{mNORMAL          , (CYAN<<4)+MAGENTA, 0, 0}     /* attrUserName      */\




//************************ Notification codes ********************************

enum NOTIFYCODES {ncNone=0,   // parent notification codes
	ncOkButton,                // OK button pressed
	ncCloseButton,             // Close button pressed
	ncCancButton,              // Cancel button pressed
	ncHScrollBar,              // horizontal scroll bar moved
   ncVScrollBar,              // vertical scroll bar moved
   ncNewButton                // new filter button
   };



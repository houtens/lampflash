/* Global definitions */

/* Cannot get the PalmChars.h stuff to work properly on both the TX and older palms */

#define MAXDISPLAYSIZE  25     /* Max number of answers to each question... for displaying */
#define MAXNOFLASHCARDS 250    /* Number of alphagram questions.  50-100 is best. */
#define COUNTFIELDSIZE 9       /* Size of the counter text field */

#define LISTSIZE 7             /* Lines in the main word solution window */
#define DBLISTMAX 9           /* Lines in the DB selection window */

/* Main Form definitions */

#define MainForm               1000  
#define MainWordList           1001  /* Answers window */
#define MainWordScroll         1002  /* Answers scrollbar */


/* Flashcard counter - "100/100" */
#define MainFlashCounter       1003
#define MainAnsCounter         1004
#define MainDBTitle            1005

/* Buttons */
#define MainAnswers            1012

#define MainNext               1013
#define MainLast               1014
#define MainClue               1015
#define MainNextAnswer         1016
#define MainDelete             1017  /* Delete the current flashcard button */

/* Letter ordering */
#define OrderAlphaPush         1020
#define OrderRandPush          1021
#define OrderVowconPush        1022
#define OrderSort              1023  /* Group buttons */



/******************************************************************************/
/* MainMenu definitions                                                       */
/******************************************************************************/

#define MainMenu              1100  

/* HELP menu */
#define HelpMenuNewDB         1111  /* "New Wordlist" */
#define HelpMenuUndelete      1112  /* Unhide the hidden flashcards */
#define HelpMenuShuffle       1113  /* Shuffle the flashcard set */
/* -- */
#define HelpMenuPrefs         1114  /* Launch prefs form */
#define HelpMenuInst          1115  /* Instructions */
#define HelpMenuAbout         1116  /* Display the about page */

/* OPTIONS menu */


/******************************************************************************/
/* PrefsForm definitions                                                       */
/******************************************************************************/

#define PrefsForm             1120

/* Buttons and triggers */
#define PrefsOKButton         1124
#define PrefsCancelButton     1125
#define ShowCounterTrig       1130
#define ShowCounterList       1131
#define HooksTrig             1132
#define HooksList             1133
#define ShowTilesTrig         1134
#define ShowTilesList         1135


/* Dictionary form definitions */
#define DictForm              1400
#define DictButtonDone        1410
#define DictButtonSearch      1411
#define DictWordField         1412
#define DictTextField         1413
#define DictInputField        1414
#define DictButtonPrev        1415
#define DictButtonNext        1416



/******************************************************************************/
/* DBForm definitions                                                       */
/******************************************************************************/

// DB form definitions
#define DBForm                1150
#define DBList                1151
#define DBButtonOK            1152
#define DBListScroll          1153
#define DBCancel              1154

#define DBMenu                1160
#define DBMenuOpts            1161
#define DBMenuOptsDelete      1162
#define DBMenuOptsBeam        1163


/******************************************************************************/
/* Confirm DELETE DBForm? definitions                                                       */
/******************************************************************************/

#define DBDeleteForm          1170
#define DBDeleteYes           1171
#define DBDeleteNo            1172
#define DBDeleteName          1173

/* String definitions */
#define VersionStr            9050
#define InstStr               9051

/* Alert Boxes */
#define DBErrorAlert          1200
#define DBTooManyAlert        1201
#define DBNotFound            1202
#define AboutAlert            1203
#define NoLFD                 1204
#define DBOrderSaveFailed     1205
#define DBTooManyQuestionsAlert  1206
#define DBOrderOpenFailed     1207
#define NoDictDatabase        1208
#define WordTooLong           1209


// Debug stuff

#define Debug                 1250

#define AllocAlert            1300
#define FreeAlert             1301

#define AllocAlertDB          1302
#define FreeAlertDB           1303
#define ResizeAlertDB         1304

#define AllocAlertPDB         1305
#define FreeAlertPDB          1306
#define ResizeAlertPDB        1307

#define AllocAlertPPA         1308
#define FreeAlertPPA          1309
#define ResizeAlertPPA        1310


#define NoDatabases           1350



/* Bitmaps */
#define BitmapA               1501
#define BitmapB               1502
#define BitmapO               1503



#define ResizeDB              1500
#define ResizePDB             1501
#define AllocPAH              1502
#define WhatNow               1503
#define ALLCARDSHIDDEN        1504


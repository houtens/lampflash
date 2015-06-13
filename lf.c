/* ----------------------------------------------------------------------------- 
   Program: LAMPFlash
   Version: 1.5.1
   
   LAMPFlash is a word flashcard program written for the Palm OS and
   is intended for use as a study aid by Scrabble(R) players.

   Author: Stewart Houten (houtens@gmail.com)
   ----------------------------------------------------------------------------- */
   
/* Include PALM OS and program specific headers */
#include <PalmOS.h>
#include <PalmChars.h>
#include <PalmNavigator.h>
#include "lf.h"


/* GLOBAL CONSTANTS */


/* My creator ID is registered with Palm via Access - see the website
   (http://www.access-company.com/developers/index.html) */
#define CREATORID        'shLF'   

/* PDB databases should be of this type - case sensitive! */
#define DBTYPE           'DATA'

/* Used to retrieve the state and prefs data */
#define PREFSID              1
#define STATEID              1

/* Update the version numbers whenever we redefine the prefs and state
 * structures otherwise we read garbage. */
#define PREFSVERSION         15
#define STATEVERSION         18

/* NOTE - if you change MAXDBTITLE you will alter the size of the prefs structure
   and so the prefs version number must be incremented. */
#define MAXDBTITLE          32  

/* Allocate memory to display this many DBs - more memory is
   allocated as needed. */
#define INITNODBS           20

/* This program is designed with a maximum length of flashcard 
   in mind.  The size of the screen and letters determines this. */
#define MAXWORDLENGTH       9

/* This value is arbitrary and defines the size of the buffers 
   used to store the front and back hooks.  Should be sufficient as
   surely no word has more than 20 front or back hooks. */
#define MAXNOHOOKS          20

/* Directional constants used in scrolling the DB list */
#define NONE                 0
#define DOWN                 1
#define UP                  -1

/* Tile and rack dimensions and positions */
#define YOFFSET             22
#define TILEWIDTH           16 
#define TILEHEIGHT          17
#define TILESPACE            2

/* no definition should be longer than this. */
#define MAXDEFLENGTH       500




/* STRUCTURE DEFINITIONS */





/* dbOrderType - Quiz order data associated with a DB. */
typedef struct
{
  Char       title[MAXDBTITLE];
  UInt16     total;
  UInt16     visible;
  Int16      order[MAXNOFLASHCARDS];   
} dbOrderType;


/* dbTitleType - Holds a DB title and is used in displaying the DB list. */
typedef struct
{
  Char     title[MAXDBTITLE];
} dbTitleType;


/* wordListType - Holds the current flashcard data (answers count, the
   words, and the hooks).  NOTE - the flashcard is stored separately in
   flashcard. */
typedef struct
{
  UInt16        count;  /* Number of answers to the current flashcard */
  Char          words[MAXDISPLAYSIZE][MAXWORDLENGTH + 1];
  Char          front[MAXDISPLAYSIZE][MAXNOHOOKS + 1];  
  Char          back[MAXDISPLAYSIZE][MAXNOHOOKS + 1];   
} wordListType;


/* prefsType - The system preferences. */
typedef struct
{
  UInt8         showcount;   /* Display the number of answers? */
  UInt8         letterorder; /* Default letter order of the flashcard */
  UInt8         showhooks;   /* Display hooks where available? */
  UInt8         showtiles;   /* Show tiles or just write the flashcard string? */
} prefsType;


/* stateType - The current state data. */
typedef struct
{
  UInt16  dbcurrec;  /* Record number of the current question */
  Char    dbname[MAXDBTITLE]; /* Title of the current database */
  UInt16  total; /* The number of records in the DB */
  UInt16  seen; /* Current flashcard number as displayed (not curr) */
  UInt16  visible; /* Total number of visible flashcards (un-hidden ones) */ 
  UInt16  curr; /* Record number of the current flashcard (not seen) */
  Int16   order[MAXNOFLASHCARDS]; /* Order values */
  UInt16  noCurrDB; /* The number of the current database */
} stateType;


/* Define a global pointer to the current form */
static FormPtr      pCurForm = NULL; 

/* Pointers to the word list - needed to display the list of 
   answers and to make the scroll bar work. */
static Char         *pMainWordListPtrArray[MAXDISPLAYSIZE];


/* The flashcard string - this can be manipulated by shuffling. */
static Char         flashcard[MAXWORDLENGTH + 1];
/* Buffer to hold the alphagram of the flashcard */
static Char         alphagram[MAXWORDLENGTH + 1];
/* The quiz data associated with the flashcard - answers count,
   answers and hooks */ 
static wordListType flash;

static Char         display[MAXDISPLAYSIZE][40 + 1];  /* flashwords */

/* Buffers to hold the text on the Prefs form pull-down menus */  
static Char         countertxt[5];    
static Char         hookstxt[5];    
static Char         showtilestxt[5];  
static Char         *counteropts[2] = { "No", "Yes" };



/* Counter for the number of LAMPFlash databases found. */
static UInt16       dbc; 

/* Number of records in the current database */
static UInt16       dbnumrec;

/* Database tiles and pointers to these - allocated dynamically below */
static dbTitleType                *db = NULL; 
static MemHandle                  dbh = NULL; 
static dbTitleType              **pdb = NULL;
static MemHandle                 pdbh = NULL;

/* Pointers to the DB titles for display and scrolling */
static Char         **pDBListPtrArray = NULL;
static MemHandle                 ppah = NULL;




/* String buffer to hold the version defined in the RCP file */
static Char         version[11];

/* Flag to indicate whether hooks are to be displayed.  If the quiz is
   not anagrams (ie Hooks) then the length of the flashcard will
   differ from the length of the answers.  This is used determine
   whether hooks should be displayed or not.  If this is true, then
   shuffling of the flashcard is also not allowed. */
static Boolean      doingHooks = false;

/* If we are restoring a previous quiz we restore things from where we
   left off.  Otherwise we reset the stats etc. */
static Boolean      restore = false;


/* Counter for the number of answers revealed by "+". */
static UInt16       revealed;
/* Counter for the number of clues that have been revealed. */
static UInt16       clues;        

/* Identifier used to determine which DB should be highlighted
   after deleting a DB. */
static UInt16       lastID;

/* Store the name of the next DB to be highlighted in the list.
   Cannot be null so it contains some arbitrary text initially. */
static Char          nextDBhighlight[MAXDBTITLE] = "a\0";

/* Define the array of rectangles that define the flashcard tiles. */
static RectangleType rack[8]; /* or rack pointer */

/* Indicates which tile is highlighted in the rack (if none is 
   selected is should be set to -1. */
static Int8          highlight;


/* File name of the lampflash metadata DB. */ 
static Char          *LFD = "lampflash.data\0";
static Char          *dictionarydb = "lfdict";


/* Size of the DB list displayed - incremented when memory is realloc()ed. */
static UInt32        dblistsize;


/* System state and preferences */
static stateType     state;
static prefsType     prefs;


/* Dictionary word for looking up. */
static Char lookup[MAXDBTITLE + 1];           /* words can be up to 32 characters */
static Char firstword[MAXDBTITLE + 1];
static Char definition[MAXDEFLENGTH+1];
static UInt16 lookuprec, lookupoffset;



/* GLOBAL FORM POINTERS */






/* Main form */

static FieldPtr       pMainFlashCounter = NULL;
static FieldPtr       pMainAnsCounter = NULL;
static FieldPtr       pMainDBTitle = NULL;
static ListPtr        pMainWordList = NULL;
static ScrollBarPtr   pMainScrollBar = NULL;
static ControlPtr     pMainAnswers = NULL;
static ControlPtr     pMainNext = NULL;
static ControlPtr     pMainLast = NULL;
static ControlPtr     pMainOrderAlphaPush = NULL;
static ControlPtr     pMainOrderRandPush = NULL;
static ControlPtr     pMainOrderVowconPush = NULL;
static ControlPtr     pMainNextAnswer = NULL;
static ControlPtr     pMainClue = NULL;
static ControlPtr     pMainDelete = NULL;

/* Dictionary form */

static FieldPtr       pDictTextField = NULL;
static FieldPtr	      pDictWordField = NULL;
static FieldPtr	      pDictInputField = NULL;


/* Preferences form */

static ControlPtr     pPrefsShowCounterTrig = NULL;
static ListPtr        pPrefsShowCounterList = NULL;
static ControlPtr     pPrefsHooksTrig = NULL;
static ListPtr        pPrefsHooksList = NULL;
static ControlPtr     pPrefsShowTilesTrig = NULL;
static ListPtr        pPrefsShowTilesList = NULL;

/* Database form */

static ListPtr        pDBList = NULL;
static ControlPtr     pDBButtonOK = NULL;
static ScrollBarPtr   pDBListScroll = NULL;

/* Delete database form */

static ControlPtr     pDBDeleteYes = NULL;
static ControlPtr     pDBDeleteNo = NULL;
static FieldPtr       pDBDeleteName = NULL;






/* These are listed here in the order they appear in this file */
static void SearchRecordByScroll(UInt8 dir);


static void DisplayLookupWord(Char *word);
static void CleanUpString(Char *str);

static void LookupDefinition(Char *word);
static UInt16 SearchRecordForWord(DmOpenRef dbref, UInt16 id);
static void ReadFirstWordInRecord(DmOpenRef dbref, UInt16 id);



/* Debugging function used to display the character code of the Palm buttons. */
static void fooWrite(UInt16 foo, UInt16 y);

// static void databaseSelect(void);

/* main form buttons control functions */
static void flashcardDoNext(void);
static void flashcardDoLast(void);
static void flashcardShowAll(void);
static void flashcardShowOne(void);
static void flashcardDelete(void);
static void flashcardShowClue(void);

/* Clean up the DB Pointer stuff */
static void CleanUpDBPointers(void);

/* Save Order Data */
static void    SaveOrderData(void);

/* Card delete functions */
static void    UndeleteAll(void);
static void    DeleteAndStop(void);
static void    DeleteAndDoNext(void);
static void    DrawBlankForm(void);

/* DB beam functions */
static Err     WriteDBData(const void* dataP, UInt32* sizeP, void* userDataP);
static Err     SendMe(void);
static Err     SendDatabase(UInt16 cardNo, LocalID dbID, Char* nameP, Char* descriptionP);

/* Tile display */
static void    DeleteDB(Char *db);
static void    WordDBScroll(Int8 dir);
static void    SortByName(dbTitleType **x, UInt16 first, UInt16 last);
static UInt16  PartitionByName(dbTitleType **y, UInt16 f, UInt16 l);
static void    SetWordOrder();
static void    ShowAnswers(UInt16 max, UInt16 clues);
static void    DoNext(void);
static void    DoLast(void);

static void    ClearFlashField(void);
static void    DrawTile(RectangleType rec, Char c, UInt8 border);
static void    SetUpFlashcardField(void);
static void    SetFlashField();
static void    SetFlashNumber(UInt16);
static void    OrderNewFlashcard(void);
static void    RandomiseFlashcard(void);
static void    VowConiseFlashcard(void);
static void    AlphagramiseFlashcard(void);
static void    ResetStats(void);
static void    ReorderFlashcards(void);
static Err     FindAllWordDBs(void);                /* check return codes */ 
static void    ShowWordDBs(void);
static Err     CountRecordsInDB(void);
static UInt16  RandomNum(UInt16 n);
static void    SetField(FieldPtr field, Char* text, UInt16 memsize);
static void    GetNewFlashcard(void);
static void    SetCounter(UInt16 c);

static void    InitMainForm(void);
static Boolean DBDeleteFormEventHandler(EventPtr event);
static Boolean PrefsFormEventHandler(EventPtr event);

static Boolean DictFormEventHandler(EventPtr event);

static Boolean DBFormEventHandler(EventPtr event);
static Boolean MainFormEventHandler(EventPtr event);
static Boolean AppEventHandler(EventPtr event);
static void    StopApplication(void);
static Err     StartApplication(void);
static void    EventLoop(void);
UInt32         PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags);






/* FUNCTIONS */

static void SearchRecordByScroll(UInt8 dir) {
  DmOpenRef dbref;
  LocalID dbid; 
  MemHandle h;
  UInt16 nrecs;
  Char *pChar, *s, *d;

  /* Open the DB and count the number of records. */
  dbid = DmFindDatabase(0, dictionarydb);
  if (dbid > 0) {
    dbref = DmOpenDatabase(0, dbid, dmModeReadOnly);
    if (dbref != NULL) {
      nrecs = DmNumRecords(dbref);
      /* Ensure we can read the record in lookuprec. */
      if (lookuprec <= nrecs) {

	if (dir > 0) {
	  h = DmQueryRecord(dbref, lookuprec);
	  if (h) {
	    pChar = MemHandleLock(h);
	    s = pChar;
	    d = firstword;
	    while ((*s != '\0') && (*s != '\t')) {
	      s++;
	      if (*s == '\t') {
		while ((*s != '\0') && (*s != '\t')) {
		  s++;
		}
	      }
	    }
	    /* should be at new word or end of file. */
	    if (*s == '\t') {
	      s++;
	      /* read the new word. */
	      while ((*s != '\0') && (*s != '\t')) {
		*d++ = *s++;
	      }
	      *d = '\0';
	      StrCopy(lookup, firstword);
	    }
	    else {
	      /* definition is in the next record */
	      /* - check the next record exists */
	      /* - if so, read the first word and definition. */
	    }

	    
	    /* Search for the next word following lookup. */
	  
	    MemHandleUnlock(h);

	  }
	}
      }
      DmCloseDatabase(dbref);
    }
  }
}



static void DisplayLookupWord(Char *word) {
  Char tmp[MAXDBTITLE + 1];
  Char *s, *d;
  
  s = word; d = tmp;
  while (*s) {
    *d++ = *s++;
  }
  *d = '\0';
  SetField(pDictWordField, tmp, MAXDBTITLE + 1);
}
    
  




static void CleanUpString(Char *str) {
  Char tmp[MAXDBTITLE+1];
  Char *s = str;
  Char *t = tmp;

  while (*s != '\0') {
    if (*s >= 'A' && *s <= 'Z') {
      *t++ = *s + 32;
    }
    if (*s >= 'a' && *s <= 'z') {
      *t++ = *s;
    }
    s++;
  }
  *t = '\0';
  StrCopy(str, tmp);
}



static void LookupDefinition(Char *word) {
  /* Vars for the dictionary database. */
  DmOpenRef dbref;
  LocalID dbid; 
  UInt16 nrecs, min = 0, max, mid, i, found = 0;
  Int16 comp;
  Char tmp[MAXDBTITLE+1];
  Char *pChar, *r, *t;
  /* safety-net in case the while loop fails to converge */
  UInt16 iterations = 0, innextrecord;

  if (StrCompare(lookup, word) != 0) {
    StrCopy(lookup, word);
  }
  
  dbid = DmFindDatabase(0, dictionarydb);
  if (dbid > 0) {
    
    dbref = DmOpenDatabase(0, dbid, dmModeReadOnly);
    
    if (dbref != NULL) {
      nrecs = DmNumRecords(dbref);
      max = nrecs - 1;
      
      /* Perform a binary search on the first entry of each record
	 until we have the record in which the lookup word is contained.
	 Variable "iterations" is used to prevent the search from running 
	 away with itself.  Having refined the search method this should
	 no longer be necessary but we'll leave it for now. */
      
      while ( (max - min) > 1) {
	iterations++;
	mid = (max + min) / 2;
	
	ReadFirstWordInRecord(dbref, mid);
	
	comp = StrCompare(lookup, firstword);
	if (comp > 0)
	  min = mid;
	else
	  max = mid;
	
	if (iterations == nrecs) break;
      }
      
      /* Found the right record - now find the definition. */ 
      
      if (iterations < nrecs) {
	/* lookup is in either min or max. */ 
	ReadFirstWordInRecord(dbref, max);
	if (StrCompare(lookup, firstword) < 0)
	  max = min;
      }
      else {
	/* the firstword binary search went wrong.  this needs an
	   error message. */
      }
      
      /* Lookup word should be in max record. */
      ReadFirstWordInRecord(dbref, max);
      /* Find the lookup word within the max record. */
      innextrecord = SearchRecordForWord(dbref, max);
      if (innextrecord == 1) {
	if ((max+1) < nrecs) {
	  SearchRecordForWord(dbref, max + 1);
	}
	else {
	  /* give the notfound message */
	  StrCopy(definition, "Not found.\0");
	}
      }


      /* Lookup word and definition text should be saved in the global definition variable. */
      DisplayLookupWord(lookup);
      SetField(pDictTextField, definition, MAXDEFLENGTH+1);
      
      /* Close the dictionary database. */
      DmCloseDatabase(dbref);
    }
  }
  else {
    /* Cannot open DB */
    FrmAlert(NoDictDatabase);
  }
}	



static UInt16 SearchRecordForWord(DmOpenRef dbref, UInt16 id) {
  MemHandle h;
  Char *pChar, *s, *d;
  Int16 comp;
  UInt16 found = 0, innextrecord = 0;
  Char tmp[32];

  h = DmQueryRecord(dbref, id);
  if (h) {
    pChar = MemHandleLock(h);
    s = pChar;

    while (found == 0) {
      d = firstword;
      lookuprec = id;
      lookupoffset = s - pChar;

      while ((*s != '\0') && (*s != '\t')) {
	*d++ = *s++;
      }
      *d = '\0';

      comp = StrCompare(lookup, firstword);
      if (comp > 0) {            /* lookup word sorts after this one. */
	s++;
	while ((*s != '\0') && (*s != '\t')) {
	  s++;       /* skim past the definition. */
	}
	if (*s == '\0') {
	  found = 1;
	  /* this means the lookup word was not in the dictionary and should
	     have been the last word in the record.  So for consistency we should 
	     display the first word in the next record. */
	  innextrecord = 1;
	}
	else {
	  s++;
	}
      }
      else {
	found = 1;
	/* similar to the above paragraph - we have travelled past where the lookup
	   word should sort and it was not found.  Thus we display the last found
	   word which should be alphabetically after the lookup word. */

	StrCopy(lookup, firstword);

	// get the definition
	d = definition;
	s++;                // advance the source pointer
	/* replace "\ and n" chars with real "\n" chars */
	while ((*s != '\0') && (*s != '\t')) {
	  if ((*s == 92) && (*(s+1) == 110)) {
	    *d++ = '\n';
	    s++; s++;
	  }
	  else {
	    *d++ = *s++;
	  }
	}
	*d = '\0';
      }
    }

    MemHandleUnlock(h);
  }
  else {
    /* Can't open the record.  Set the definition text to the lookup word
       and popup an error form. */
  }

  return innextrecord;
}


static void ReadFirstWordInRecord(DmOpenRef dbref, UInt16 id) 
{
  MemHandle h;
  Char *pChar, *s, *d;
  
  h = DmQueryRecord(dbref, id);
  if (h) {
    pChar = MemHandleLock(h);
    s = pChar;
    d = firstword;
    while ((*s != '\0') && (*s != '\t')) {
      *d++ = *s++;
    }
    *d = '\0';
    MemHandleUnlock(h);
  }
 else {
    StrCopy(firstword, "\0");
 }

}


static void fooWrite(UInt16 foo, UInt16 y) 
{
  Char bar[10];
  StrIToA(bar, foo);
  StrCat(bar, "\0");
  WinDrawChars(bar, StrLen(bar), 60, y);
}



static void flashcardDoNext(void)
{
  if (state.visible > 1)
    DoNext();
}

static void flashcardDoLast(void)
{
  if (state.visible > 1)
    DoLast();
}


static void flashcardShowAll(void)
{
  if (state.visible > 0)
    {
      if (revealed < flash.count)
	{
	  clues = 0;
	  revealed = flash.count;
	  ShowAnswers(revealed, clues);
	}
    }
}

static void flashcardShowOne(void)
{
  if (state.visible > 0)
    {
      if (revealed < flash.count)
	{
	  clues = 0;
	  revealed++;
	  ShowAnswers(revealed, clues);
	}
    }
}

static void flashcardShowClue(void)
{
  if (revealed < flash.count)
    {
      if (!doingHooks) 
	{
	  if (clues < StrLen(flash.words[0]))
	    clues++;
	}
      ShowAnswers(revealed, clues);
    }
}



/* Hide the question - this only works when all the answers
   have been revealed.  Further to Tim's suggestions - it 
   might be nice to implement  grayout of the button when
   it is disabled although this goes against the advice
   of the PalmOS UI guide. */
static void flashcardDelete(void)
{
  if (state.visible > 1)
    {
      if (revealed == flash.count)
	DeleteAndDoNext();
    }
  else if (state.visible == 1)
    {
      if (revealed == flash.count)
	DeleteAndStop();
    }
}




static void CleanUpDBPointers(void) 
{
  /* Called on exitiing the DB page.  Free memory and
     clean up the pointers. */

  MemHandleUnlock(dbh);
  MemHandleFree(dbh);
  dbh = NULL;
  db = NULL;
  
  MemHandleUnlock(pdbh);
  MemHandleFree(pdbh);
  pdbh = NULL;
  pdb = NULL;
  
  MemHandleUnlock(ppah);
  MemHandleFree(ppah);
  ppah = NULL;
  pDBListPtrArray = NULL;
}



static void SaveOrderData(void)
{

  UInt16           index;
  UInt16         records;
  UInt16               i;
  
  MemHandle h;
  dbOrderType        old;
  dbOrderType      *oldp;
  dbOrderType        new;
  dbOrderType      *newp;
  
  LocalID           dbID;
  DmOpenRef        dbRef;
  
  oldp = &old;  /* Necessary?  It gets reset below... */
  
  /* Find the data DB */
  dbID = DmFindDatabase(0, LFD);
  if (dbID)
    {
      /* Open the data DB */
      dbRef = DmOpenDatabase(0, dbID, dmModeReadWrite);
      if (dbRef)
	{
	  /* Read the number of records in the DB */
	  records = DmNumRecords(dbRef);
	  /* Read the existing data... 
	   * until we find one matching the current set, then delete it
	   */
	  if (records > 0)
	    {
	      for (i = 0; i < records; i++)
		{
		  h = DmQueryRecord(dbRef, i);
		  if (h)
		    {
		      oldp = (dbOrderType *) MemHandleLock(h);
		      MemHandleUnlock(h);
		      
		      if (StrCompare(oldp->title, state.dbname) == 0)
			{
			  /* Found the old data.  Delete it */
			  DmDeleteRecord(dbRef, i);
			  break;
			}
		    }
		}
	    }
	  
	  /* Copy current data into new */
	  StrCopy(new.title, state.dbname);
	  new.total = state.total;
	  for (i = 0; i < state.total; i++)
	    new.order[i] = state.order[i];
	  
	  /* Find the last record index */
	  index = dmMaxRecordIndex;	    
	  
	  /* Create a new record and write data */
	  h = DmNewRecord(dbRef, &index , sizeof(dbOrderType));
	  newp = MemHandleLock(h);
	  DmWrite(newp, 0, &new, sizeof(dbOrderType));
	  MemHandleUnlock(h);
	  DmReleaseRecord(dbRef, index, true);
	  
	  DmCloseDatabase(dbRef);
	}
      else
	{
	  /* Cannot OPEN the "lampflash.data" DB by name */
	  FrmAlert(DBOrderOpenFailed);
	}
      
    }
  else
    {
      /* Cannot FIND the "lampflash.data" DB by name */
      FrmAlert(DBOrderSaveFailed);
    }
}


static void UndeleteAll(void)
{
  /* This is duplication of effort but this thing has got become
     so spaghetti-fied I can't bare to try to unravel the thing. */
  UInt16 i;
  
  state.visible = state.total;
  state.seen = 0;
  
  /* This counter is 1 - N+1 while the array reference is 
     obviously 0 - N.  This has tripped me up once! */
  state.curr = state.seen + 1;  
  state.dbcurrec = state.seen;
  restore = false; 
  
  /* Cycle through order[] and make all values positive (all positive
     referenced numbers are shown - negative values are hidden */
  
  for (i = 0; i < state.total; i++)
    {
      if (state.order[i] < 0)
	{
	  state.order[i] = Abs(state.order[i]);
	}
    }
  
  /* Get the new flashcard and display it */
  
  GetNewFlashcard();
  OrderNewFlashcard();
  SetWordOrder();
  SetFlashNumber(state.curr);
  SetCounter(flash.count);
  SetUpFlashcardField();
  SetFlashField();  
}



static void DrawBlankForm(void)
{
  static Char str[COUNTFIELDSIZE + 1];
  
  SetFlashNumber(0);
  StrCopy(flashcard, "");
  SetFlashField();
  StrCopy(str, "\0");
  SetField(pMainAnsCounter, str, COUNTFIELDSIZE + 1);
  
  /* ARV */
  SetWordOrder(); 
  
  FrmDrawForm(pCurForm);
  ShowAnswers(NONE, 0);
  
  /* Display the title field */
  FldSetTextPtr(pMainDBTitle, state.dbname);
  FldDrawField(pMainDBTitle);
  
  FrmAlert(ALLCARDSHIDDEN);  
}



static void DeleteAndStop()
{     
  /* Special function to get around a tricky issue.  This is pretty
     inelegant but it works.  Perhaps something to look over when
     I have more time. */
  
  /* This function is called to delete the last flashcard and ensure that
     the counters and displays are blanked. */
  
  state.visible = 0;
  state.order[state.seen] = (-1) * state.order[state.seen];
  state.curr = 0;
  DrawBlankForm();
}


static void DoShuffle()
{
  /* We must ONLY call DoShuffle if we have more than one unseen */
  
  UInt16 i = 0, r, t;
  
  /* Shuffle the order */
  for (; i < state.total; i++)
    {
      r = RandomNum(state.total);
      t = state.order[i];
      state.order[i] = state.order[r];
      state.order[r] = t;
    }
  
  /* Reset the order counter */
  state.seen = 0; 
  
  /* Find the first undeleted (positive) entry */
  while (state.order[state.seen] < 0)
    {
      if (state.seen < state.total - 1)
	state.seen++;
    }		    
  
  /* Update the current counter */
  state.dbcurrec = state.seen + 1;
  state.curr = 1;
  
  
  /* Get a new flashcard and update the display and counters */
  ShowAnswers(NONE, 0);
  GetNewFlashcard();
  OrderNewFlashcard();
  SetFlashNumber(state.curr);
  SetFlashField(flashcard);
  SetCounter(flash.count);
  
}




static void DeleteAndDoNext()
{
  /* Delete the question by making its order reference negative */
  
  /* Check that it isn't already negative.  This probably isn't 
     necessary but we do it anyway. */
  if (state.order[state.seen] > 0)
    {
      /* Negate order reference */
      state.order[state.seen] = (-1) * state.order[state.seen];
      
      if (state.visible == state.curr)
	{
	  /* Reduce the counter of visible flashcards */
	  state.curr--;
	  
	  /* Find the last (previous) undeleted flashcard */ 
	  while(state.order[state.seen] < 0)
	    state.seen--;
	}
      else
	{
	  /* Find the next undeleted flashcard */
	  while(state.order[state.seen] < 0)
	    state.seen++;
	  
	}
      
      /* Always update the visible counter */
      state.visible--;
      
      /* Get a new flashcard and Update the displays */
      ShowAnswers(NONE, 0);
      GetNewFlashcard();
      OrderNewFlashcard();
      SetFlashField(flashcard);
      SetFlashNumber(state.curr);
      SetCounter(flash.count);
    }
}



/* Callback function for sending DB */
Err WriteDBData(const void* dataP, UInt32* sizeP, void* userDataP)
{
  Err err;
  *sizeP = ExgSend((ExgSocketPtr)userDataP, (void*)dataP, *sizeP, &err);
  return err;
}


static Err SendDatabase(UInt16 cardNo, LocalID dbID, Char* nameP, Char* descriptionP)
{
  Err err;
  ExgSocketType exgSocket;
  
  MemSet(&exgSocket, sizeof(exgSocket), 0);
  exgSocket.description = descriptionP;
  exgSocket.name = nameP;
  
  err = ExgPut(&exgSocket);
  if (!err)
    {
      err = ExgDBWrite(WriteDBData, &exgSocket, NULL, dbID, cardNo);
      err = ExgDisconnect(&exgSocket, err);
    }
  return err;
}


/* Send the database via infrared - Lifted from LAMPWords */
static Err SendMe(void)
{
  Err err;
  LocalID              dbID;
  Char    tname[MAXDBTITLE + 3];
  
  /* Get the DB ID by name */ 
  dbID = DmFindDatabase(0, state.dbname);
  
  if (dbID)
    {
      /* Important to add the file extension to the DB name */
      StrCopy(tname, state.dbname);
      StrCat(tname, ".pdb");
      err = SendDatabase(0, dbID, tname, state.dbname);
    }
  else
    {
      err = DmGetLastErr();
    }
  return err;
}





/* Delete a database and LFD entry */
static void DeleteDB(Char *db)
{
  LocalID              dbID;
  DmOpenRef           dbRef;
  UInt16               n, i;
  MemHandle               h;
  dbOrderType            *p;
  
  /* Delete flashcard database */ 
  
  dbID = DmFindDatabase(0, db);
  if (dbID)
    DmDeleteDatabase(0, dbID);
  
  
  /* Add some code here to handle the deletion of the old -asdf titled
     databases that held meta-data about the flashcards in an old version */ 
  
  /* Find the LFD entry and delete it */
  
  dbID = DmFindDatabase(0, LFD);
  if (dbID)
    {
      dbRef = DmOpenDatabase(0, dbID, dmModeReadWrite);
      if (dbRef)
	{
	  n = DmNumRecords(dbRef);
	  if (n > 0)
	    {
	      for (i = 0; i < n; i++)
		{
		  h = DmQueryRecord(dbRef, i);
		  if (h)
		    {
		      p = (dbOrderType *) MemHandleLock(h);
		      MemHandleUnlock(h);
		      
		      if (StrCompare(p->title, db) == 0)
			{
			  DmDeleteRecord(dbRef, i);
			  break;
			}
		      //DmReleaseRecord(dbRef, i, false);
		    }
		}
	    }
	  DmCloseDatabase(dbRef);
	}
    }
}




/* Complicated fudge created to scroll the DB list */
static void WordDBScroll(Int8 dir)
{
  UInt16 bar;
  UInt16 top;
  
  bar = LstGetSelection(pDBList);
  top = LstGetTopItem(pDBList);
  
  if (dir == DOWN)
    {
      if (bar < dbc - 1)
	{
	  bar++;
	  if (bar == top + DBLISTMAX)
	    top++;
	  if (bar == top + DBLISTMAX - 1)
	    top++;
	}
    }
  if (dir == UP)
    {
      if (bar > 0)
	{
	  bar--;
	  if (bar == top - 1)
	    top--;
	  if (bar == top && bar > 0)
	    top--;
	}
    }
  
  LstSetTopItem(pDBList, top);
  LstSetSelection(pDBList, bar);
  LstDrawList(pDBList);
  
  top = LstGetTopItem(pDBList);
  
  SclSetScrollBar(pDBListScroll, (dbc - top) <= DBLISTMAX ? dbc - DBLISTMAX : top, 
		  0, dbc <= DBLISTMAX ? 0 : dbc - DBLISTMAX, DBLISTMAX);
  
} 




/* Quicksort the DB names for display on DB form */
static void SortByName(dbTitleType **x, UInt16 first, UInt16 last)
{
  dbTitleType *tmp;
  UInt16 pivIndex = 0;
  if (first < last)
    {
      pivIndex = PartitionByName(x, first, last);
      if (pivIndex > 0)
	SortByName(x, first, (pivIndex - 1));
      if (pivIndex < last)
	SortByName(x, (pivIndex + 1), last);
    }
}


/* Quicksort algorithm partition */
static UInt16 PartitionByName(dbTitleType **y, UInt16 f, UInt16 l)
{
  UInt16 up, down;
  dbTitleType *piv = y[f];
  dbTitleType *tmp;
  up = f;
  down = l;
  do {
    while (StrCompare(y[up]->title, piv->title) <= 0 && up < l)
      up++;
    while (StrCompare(y[down]->title, piv->title) > 0)
      down--;
    if (up < down)
      {
	tmp = y[up];
	y[up] = y[down];
	y[down] = tmp;
      }
  } while (down > up);
  y[f] = y[down];
  y[down] = piv;
  
  return down;
}




static void SetWordOrder()
{
  if (prefs.letterorder == 0)
    CtlSetValue(pMainOrderAlphaPush, 1);
  if (prefs.letterorder == 1)
    CtlSetValue(pMainOrderRandPush, 1);
  if (prefs.letterorder == 2)
    CtlSetValue(pMainOrderVowconPush, 1);
}



/* Display the flashcard solutions */
static void ShowAnswers(UInt16 max, UInt16 clues)
{
  UInt16 d, j;
  UInt16 drawmax;
  Char *p, *s;
  Char tmp[10] = "";
  
  /* We first set up an array of pointers to the list of answers 
     displayed on the main form */ 
  
  /* But we piggy-back the loop to format the actual text to be 
     displayed - the answers, spacing and hooks etc. */
  
  for (d = 0; d < max; d++)
    {
      /* Copy the answers */
      StrCopy(display[d], flash.words[d]);
      
      /* Add a small space before displaying the hooks */
      StrCat(display[d], "\x19\x19");     
      
      /* Add the hooks (if prefs is set) */
      if (prefs.showhooks) 
	{
	  StrCat(display[d], flash.front[d]);
	  if (StrLen(flash.front[d]) > 0 || StrLen(flash.back[d]) > 0)
	    {
	      StrCat(display[d], "-");
	    }
	  StrCat(display[d], flash.back[d]);
	}
      
      /* And actually set up the pointers */
      pMainWordListPtrArray[d] = display[d];
    }
  
  /* Declare the number of lines in the list */ 
  drawmax = max;
  
  if (clues > 0 && max < flash.count)
    {
      p = flash.words[max];
      s = tmp;
      for (j = 0; j < clues; j++)
	*s++ = *p++;
      *s = '\0';
      
      StrCopy(display[drawmax], tmp);
      pMainWordListPtrArray[drawmax] = display[drawmax];
      /* Update the number of clues displayed so far*/
      drawmax++;                 
    }
  
  /* Finally, show the words and set the list scrollbar */
  
  LstSetListChoices(pMainWordList, pMainWordListPtrArray, drawmax);
  LstSetTopItem(pMainWordList, drawmax <= LISTSIZE ? 0 : drawmax - LISTSIZE);
  LstSetSelection(pMainWordList, noListSelection);
  LstDrawList(pMainWordList);
  SclSetScrollBar(pMainScrollBar, drawmax <= LISTSIZE ? 0 : drawmax - LISTSIZE, 0,
		  drawmax <= LISTSIZE ? 0 : drawmax - LISTSIZE, LISTSIZE);
    
}


/* Handle looping around the end of the flashcard set and 
   also hidding the flashcards that are deleted. */
static void DoNext()
{
  state.seen = (state.seen + 1) % (state.total);
  while (state.order[state.seen] < 0)
    {
      state.seen = (state.seen + 1) % (state.total);
    }		    
  
  if (state.order[state.seen] > 0)
    {
      if (state.curr < state.visible)
	{
	  state.curr++;
	}
      else
	{
	  state.curr = 1;
	}
    }
  else
    {
      /* Add some code here to handle the unlikely event */
      ;
    }
  
  /* Get a new flashcard and update the display and counters */
  ShowAnswers(NONE, 0);
  GetNewFlashcard();
  OrderNewFlashcard();
  SetFlashNumber(state.curr);
  SetFlashField(flashcard);
  SetCounter(flash.count);
}



/* Similar to DoNext() above */
static void DoLast()
{
  if (state.seen <= 0) state.seen = state.total - 1;
  else state.seen--;
  
  while (state.order[state.seen] < 0)
    {
      if (state.seen <= 0) 
	{
	  state.seen = state.total - 1;
	}
      else
	{
	  state.seen--;
	}
    }		    
  
  if (state.curr > 1)
    {
      state.curr--;
    }
  else
    {
      state.curr = state.visible;
    }
  
  /* Get the next flashcard and update the display */
  GetNewFlashcard();
  OrderNewFlashcard();
  SetFlashNumber(state.curr);
  ShowAnswers(NONE, 0);
  SetFlashField(flashcard);
  SetCounter(flash.count);
}


static void ClearFlashField()
{
  RectangleType r;
  
  /* Clear a big rectangle of full screen width by tile height.  Not
     pretty but it works. */ 
  
  r.topLeft.x = 0;
  r.topLeft.y = YOFFSET;
  r.extent.x = 159;
  r.extent.y = TILEHEIGHT;
  WinEraseRectangle(&r, 0);
  
}

static void DrawTile(RectangleType rec, Char c, UInt8 border)
{
  RectangleType rp = rec;
  FontID  oldfont;
  
  /* Draw a black rectangle */
  WinDrawRectangle(&rp, 3);
  
  /* Resize the rectangle by one pixel on each side 
     and move the rectangle so it is centred in the middle
     of the one drawn above. */
  rp.topLeft.x ++;
  rp.topLeft.y ++;
  rp.extent.x -= 2;
  rp.extent.y -= 2;
  
  /* Remove the inner rectangle - or draw a while rectangle */
  if (!border)
    WinEraseRectangle(&rp, 3);
  
  /* The fonts don't display the way we'd like so they 
     may need shifting a little.  Offending letters are those
     of the new Collins word WAI (meaning water). */
  if (c == 'I') rp.topLeft.x++;                
  if (c == 'A' || c == 'W') rp.topLeft.x--;    
  
  /* Save the current font and reset the font to largeBold */ 
  oldfont = FntSetFont(largeBoldFont);
  
  /* Draw the letter in the rectangle */
  if (border)
    WinDrawInvertedChars(&c, 1, rp.topLeft.x + 3, rp.topLeft.y);
  else
    WinDrawChar(c, rp.topLeft.x + 3, rp.topLeft.y);
  
  /* Reset the current font to whatever it was. */ 
  FntSetFont(oldfont);
  
}


static void SetUpFlashcardField() 
{
  UInt8 xoffset, i, cardlen;
  
  /* xoffset is the midpoint minus half the width of the rack and
     we assume the X midpoint is at 79 (or so). */
  
  cardlen = StrLen(flashcard);
  xoffset = 79 - cardlen * (TILEWIDTH + TILESPACE) / 2;
  
  for (i = 0; i < cardlen ; i++)
    {
      rack[i].topLeft.x = xoffset + i * (TILEWIDTH + TILESPACE);
      rack[i].topLeft.y = YOFFSET;
      rack[i].extent.x = TILEWIDTH;
      rack[i].extent.y = TILEHEIGHT;
    }
}




/* Update the flashcard display */
static void SetFlashField()
{
  /* Get the length of the flashcard */
  UInt16 cardlen = StrLen(flashcard);
  UInt8 i;
  FontID oldfont;

  if (prefs.showtiles != 0)
    {
      /* Draw the rack tiles individually */
      for (i = 0; i < cardlen; i++)
	DrawTile(rack[i], flashcard[i], 0);
    }
  else
    {
      ClearFlashField();
      oldfont = FntSetFont(largeBoldFont);
      WinDrawChars(flashcard, StrLen(flashcard), 79 - 4 * StrLen(flashcard), YOFFSET);
      FntSetFont(oldfont);
    }	     
}



/* Set the display order of the flashcard according to the prefs */
static void OrderNewFlashcard()
{
  switch (prefs.letterorder)
    {
    case 0:
      AlphagramiseFlashcard();
      break;
    case 1:
      RandomiseFlashcard();
      break;
    case 2:
      VowConiseFlashcard();
      break;
    default:
      break;
    }
}


/* Randomise the tiles of the flashcard */
static void RandomiseFlashcard()
{
  UInt8 i, r, len = StrLen(flashcard);
  Char tmp;  /* Single character buffer */
  
  if (!doingHooks)
    {
      for (i = 0; i < len; i++)
	{
	  tmp = flashcard[i];
	  r = RandomNum(len);
	  flashcard[i] = flashcard[r];
	  flashcard[r] = tmp;
	}
    }
}



/* Reorder the flashcard as Vowels-Conosonants */
static void VowConiseFlashcard()
{
  
  Char tmp[StrLen(flashcard)];
  Boolean noV = false;
  UInt8 i, j = 0;
  
  if (!doingHooks)
    {
      for (i = 0; i < StrLen(flashcard); i++)
	{
	  if ( alphagram[i] == 65 ||
	       alphagram[i] == 69 ||
	       alphagram[i] == 73 ||
	       alphagram[i] == 79 ||
	       alphagram[i] == 85 )
	    {
	      flashcard[j] = alphagram[i];
	      j++;
	    }
	}
      
      for (i = 0; i < StrLen(flashcard); i++)
	{
	  if ( alphagram[i] != 65 &&
	       alphagram[i] != 69 &&
	       alphagram[i] != 73 && 
	       alphagram[i] != 79 && 
	       alphagram[i] != 85 )
	    {
	      flashcard[j] = alphagram[i];
	      j++;
	    }
	}
      
    }
}


/* Create the flashcard alphagram */
static void AlphagramiseFlashcard()
{
  /* The flashcard is in alphagram form in the DB and this is still buffered so 
     just copy it. */
  
  if (!doingHooks)
    {
      StrCopy(flashcard, alphagram);
    }
}




/* Reset the quiz stats values */
static void ResetStats()
{
  UInt16       i, t, r;
  LocalID        dbID;
  UInt16      records;
  
  dbOrderType *  oldp;
  DmOpenRef     dbRef;
  MemHandle         h;
  
  Boolean    recovered = false;
  

  /* Does the state.dbname have an entry in LFD */
  dbID = DmFindDatabase(0, LFD);
  if (dbID)
    {
      /* Open the data DB */
      dbRef = DmOpenDatabase(0, dbID, dmModeReadOnly);
      if (dbRef)
	{
	  /* Read the number of records in the DB */
	  records = DmNumRecords(dbRef);
	  if (records > 0)
	    {
	      for (i = 0; i < records; i++)
		{
		  h = DmQueryRecord(dbRef, i);
		  if (h)
		    {
		      oldp = (dbOrderType *) MemHandleLock(h);
		      MemHandleUnlock(h);
		      if (StrCompare(oldp->title, state.dbname) == 0)
			{
			  StrCopy(state.dbname, oldp->title);
			  state.total = oldp->total;
			  for (i = 0; i < state.total; i++)
			    state.order[i] = oldp->order[i];
			  recovered = true;
			  break;
			}
		    }
		}
	    }
	  DmCloseDatabase(dbRef);
	}		
    }
  
  if (recovered)
    {
      /* Set up peripheral data and the order */
      /* state.total == dbnumrec; */
      state.visible = 0;
      for (i = 0; i < state.total; i++)
	if (state.order[i] > 0)
	  state.visible++;
      
      state.seen = 0;
      state.curr = state.seen + 1;
      if (state.visible > 0)
	while(state.order[state.seen] < 0)
	  state.seen++;
      state.dbcurrec = state.seen;
    }
  else
    {
      /* Defaults here are set when we cannot find data DB */
      state.total = dbnumrec;
      state.visible = state.total;
      
      state.seen = 0;
      state.curr = state.seen + 1;
      
      state.dbcurrec = 0;
      
      /* Set up sequential order */
      for (i = 0; i < state.total; i++)
	state.order[i] = i + 1;               /* 1 - max */
      
      /* Randomise the selection */
      
      /* is this the right thing to do here? 
	 don't suppose it does any harm */
      for (i = 0; i < state.total; i++) 
	{
	  r = RandomNum(state.total);
	  t = state.order[i];
	  state.order[i] = state.order[r];
	  state.order[r] = t;
	}
      
    }
}

static void ReorderFlashcards()
{
  UInt16 i;
  Int16 tmp[MAXNOFLASHCARDS];
  Int16 t, r;
  
  /* 
   * Order the flashcards sequentially or randomly 
   */
  
  /* Set up Random order */
  for (i = 0; i < state.total; i++)
    {
      r = RandomNum(state.total);
      t = state.order[i];
      state.order[i] = state.order[r];
      state.order[r] = t;
    }
  
  
  /* Now point to the first flashcard and then step forward
   * until we find one that has a positive order[].
   */
  
  state.seen = 0;
  while (state.order[state.seen] < 0)
    state.seen++;
  
  /* Counter value is one greater than seen. */
  state.curr = 1;
  state.dbcurrec = state.seen;
  
}




static Err FindAllWordDBs()
{
  /* 
     Search through all databases by and retrieve their IDs and titles.
     If they aren't suplementary data then add them to the list to be displayed 
  */
  
  DmSearchStateType   searchState;
  UInt32                     type;
  UInt16                   cardNo;
  LocalID                    dbID;
  Err                         err;
  Err                   resizeErr;  /* Has the memory resize failed? */  
  Char      tmpdbname[MAXDBTITLE];  /* Tmp name of the current database */
  UInt16                cardcount;  /* number of flashcards in the db */
  Char               counttxt[10];  /* tmp buffer to hold counter text */
  
  
  /* 
     Free DB and PDB before trying to allocate memory - this may be used as
     a callback function!
  */
  
  if (dbh) {
    MemHandleUnlock(dbh);
    MemHandleFree(dbh);
    dbh = NULL; db = NULL;
  }
  
  if (pdbh) {
    MemHandleUnlock(pdbh);
    MemHandleFree(pdbh);
    pdbh = NULL; pdb = NULL;
  }
  
  if (ppah) {
    MemHandleUnlock(ppah);
    MemHandleFree(ppah);
    ppah = NULL;
  }
  
  
  /* Allocate an initial block of memory to hold the array of DB names
     and an array of pointers to each of these PDB. */ 
  
  dbh = MemHandleNew(dblistsize * sizeof(dbTitleType));
  db = (dbTitleType *) MemHandleLock(dbh);
  
  pdbh = MemHandleNew(dblistsize * sizeof(dbTitleType *));
  pdb = (dbTitleType **) MemHandleLock(pdbh);
  
  /* On any Palm this should never be too much memory to ask for.
     Hopefully this will never fail for reason of memory shortage. */
  
  /* Cycle through all the databases and get their titles and IDs 
     allocating more memory if requried. */
  
  
  /* Reset the database counter */
  dbc = 0;
  
  err = DmGetNextDatabaseByTypeCreator(true, &searchState, DBTYPE, CREATORID,
				       false, &cardNo, &dbID);
  
  if (err == errNone)
    {
      DmDatabaseInfo(0, dbID, tmpdbname, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
      
      
      /* Ignore the old metadata files appended with "asdf".  It was 
	 very bad practice and they really should be deleted. */
      if (StrCompare(tmpdbname, LFD) && (StrStr(tmpdbname, "asdf") == NULL))
	{
	  StrCopy(db[dbc].title, tmpdbname);
	  dbc++;
	}
      
      
      resizeErr = 0;
      
      while(((DmGetNextDatabaseByTypeCreator(false, &searchState, DBTYPE, CREATORID,
					     false, &cardNo, &dbID) == errNone) && (resizeErr == 0)))
	{
	  DmDatabaseInfo(0, dbID, tmpdbname, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	  
	  
	  /* Again, if we find a "asdf" metadata file then ignore it.  See above. */
	  if (StrCompare(tmpdbname, LFD) && (StrStr(tmpdbname, "asdf") == NULL)) 
	    {
	      /* If we have filled the DB list buffer then attempt to resize it */
	      if (dbc == dblistsize)
		{
		  /* Unlock the memory handles before resizing */
		  MemHandleUnlock(dbh);
		  MemHandleUnlock(pdbh);
		  dblistsize += 10;

		  resizeErr = MemHandleResize(dbh, (UInt32) dblistsize * sizeof(dbTitleType));
		  
		  if (resizeErr == 0)
		    {
		      resizeErr = MemHandleResize(pdbh, (UInt32) dblistsize * sizeof(dbTitleType *));
		    }					    
		  else
		    {
		      FrmAlert(ResizeDB);
		    }
		  
		  /* Whether the resize failed or not we still need to 
		     lock the memory handles */
		  db = (dbTitleType *) MemHandleLock(dbh);
		  pdb = (dbTitleType **) MemHandleLock(pdbh);
		  
		  if (resizeErr == memErrNotEnoughSpace)
		    FrmAlert(WhatNow);
		}
	      
	      /* If we had to resize the list memory blocks then let's make
		 sure that this was successful.  If not we don't want to do 
		 string copying into space we don't have.
	      
	     It shouldn't matter if DB has been resized but PDB has not.  
		 The net result is that we have a failure to resize and we cannot 
		 copy the new DB name. */ 
	      
	      if (resizeErr == 0)
		{
		  /* No resize necessary - or resize was successful */
		  StrCopy(db[dbc].title, tmpdbname);
		  dbc++;
		  
		}
	      else
		{
		  FrmAlert(ResizePDB);
		  /* Failed to resize the chunk - I assume this means that 
		     the chunk remains a) the same size, and b) unchanged. */
		  
		  /* Our list is actually not as big as we had though so
		     reduce the size counter. */
		  dblistsize -= 10;
		}
	    }
	}
      
      /* Potential Bug - If we failed to resize the DB and PDB blocks
	 above then we will probably fail to allocate PPAH, no? */
      
      
      
      /* Declare memory for the pDBListPtrArray */
      ppah = MemHandleNew(dblistsize * sizeof(Char *));
      
      if (ppah != NULL)
	{
	  pDBListPtrArray = (Char **) MemHandleLock(ppah); 
	}
      else
	{
	  FrmAlert(AllocPAH);
	  /* we have a problem */
	  ;
	}	    
      
    }
  else
    {
      /* Just a precaution */ 
      ppah = MemHandleNew(dblistsize * sizeof(Char *));
      pDBListPtrArray = (Char **) MemHandleLock(ppah); 
    }
  
  
  return err;
}	     







static void ShowWordDBs()
{
    UInt16 d;     /* Loop counter */
    UInt16 top;  /* used to adjust the scrollbar */

    lastID = 0;  /* Used to highlight the last played DB */

    for (d = 0; d < dbc; d++)
	{
	    pdb[d] = &db[d];
	}

    if (dbc > 0) SortByName(pdb, 0, (dbc - 1)); 

    /* Set up the array of pointers to correctly guessed words.
       LstSetListChoices requires an array of pointers */

    for (d = 0; d < dbc; d++)
	{
	    pDBListPtrArray[d] = pdb[d]->title;
	    if (StrCompare(pdb[d]->title, state.dbname) == 0)
		lastID = d;
	}

    /* Define the word lists to be displayed.  Note, by default no item
       is selected and in this particular list the top item should
       always be item 0 */
    
    LstSetListChoices(pDBList, pDBListPtrArray, dbc);

    if (dbc > 0) {
	LstSetSelection(pDBList, lastID);

	if (dbc <= DBLISTMAX)
	    LstSetTopItem(pDBList, lastID);
	else
	    LstSetTopItem(pDBList, dbc - lastID <= DBLISTMAX - 1 ? dbc - DBLISTMAX : lastID > 0 ? lastID - 1 : 0);
    }    
    else {
	LstSetSelection(pDBList, noListSelection);
    }	

    LstDrawList(pDBList);
    
    if (dbc > 0)
      {
	/* Draw the scrollbar */
	top = LstGetTopItem(pDBList);
	SclSetScrollBar(pDBListScroll, top, 0, dbc <= DBLISTMAX ? 0 : dbc - DBLISTMAX, DBLISTMAX);
      }
    
    /* Throw an error */
    if (dbc == 0) FrmAlert(NoDatabases);
    
}





static Err CountRecordsInDB()
{
    /* Error checking required */
    Err         err = 0;
    UInt16            i;
    MemHandle         h;       /* Memory handle to the flashcard DB */
    DmOpenRef     dbRef;       /* Open ref to the flashcard DB */
    LocalID        dbID;       /* LocalID of the open DB */


    /* Adding code to create the "lampflash.data" database 
       if it doesn't exist.  See below.*/

    
    dbID = DmFindDatabase(0, state.dbname);  /* Get dbID of named database */ 
    
    if (dbID > 0)
	{
	    dbRef = DmOpenDatabase(0, dbID, dmModeReadOnly);
	    
	    if (dbRef)
		{
		    /* Read the number of records in the DB */
		    dbnumrec = DmNumRecords(dbRef);
		    DmCloseDatabase(dbRef);
		}

	    /* Check that the DB does not contain too many records */

	    if (dbnumrec > MAXNOFLASHCARDS)
		{
		    /* Too many questions in the database */
		    err = 1;
		}

	}
    else
	{   
	    /* Failed to open DB */
	    restore = false;
	    err = 1;
	}	    

    /* Check if lampflash.data exists - if not, create it */
    if (err == 0)
	{
	    dbID = DmFindDatabase(0, LFD);  /* Does the LFD exist? */
	    if (dbID <= 0)
		{
		    /* No warning required */
		    DmCreateDatabase(0, LFD, CREATORID, DBTYPE, false);
		}
	}


    return err;
}




/*
 * RandomNum()
 *
 * Parameters: n - Upper limit of the random number
 * Returns:    Random number
 */ 
static UInt16 RandomNum(UInt16 n)
{
    return SysRandom(0) / (1 + sysRandomMax / n);
}






/*
 * SetField()
 *
 * Parameters: field - Pointer to the field control to set
 *             text - The text to set
 *             memsize - Maximum size of the field's buffer
 * Returns:    Nothing
 */
static void SetField(FieldPtr field, Char* text, UInt16 memsize)
{
    MemHandle mem;
    Char* ptr;
    
    mem = FldGetTextHandle(field);
    
    if (mem == NULL)
	{
	    mem = MemHandleNew(memsize);
	}
    
    if (mem != NULL)
	{
	    /* Last-ditch attempt to make this work. */
	    if (MemHandleSize(mem) < memsize)
		{
		    if (MemHandleResize(mem, memsize) != 0)
			{
			    ErrDisplay("Can't resize field handle for text set.");
			    mem = NULL;
			}
		}
	}
    else
	{
	    ErrDisplay("Can't create field handle for text set.");
	}
    
    if (mem != NULL)
	{
	    /* This is straight out of the Palm OS Reference Manual. */
	    FldSetTextHandle(field, NULL);
	    ptr = (Char*) MemHandleLock(mem);
	    if (ptr != NULL)
		{
		    StrCopy(ptr, text);
		    MemHandleUnlock(mem);
		}
	    
	    FldSetTextHandle(field, mem);
	    FldSetInsertionPoint(field, StrLen(text));
	    FldDrawField(field);
	}
    
}




/* GetNewFlashcard()

   Parameters: None
   Returns:    Nothing */

static void GetNewFlashcard()
{
    /* Some error checking required */

    MemHandle          h; 
    DmOpenRef      dbRef;
    LocalID         dbID;
    Char        tmp[250]; /* buffer to hold the record (flashcard, answers and hooks) */

    /* Loop counters */
    UInt16        d = 0; /* Loop counter */
    UInt16       dd = 0; /* loop counter */

    /* Pointers used to traverse the input string */
    Char* s;
    Char* t;
    
    /* When restore is true we are about to select the first flashcard
       after powering-on.  The dbcurrec will have been recovered from
       the saved state variables.  

       If so, we already have state.dbcurrec.
    */

    if (!restore) 
	{   /* Get the record number of the new flashcard is one less 
	       then the number in the order array.  NB.  The order array
	       potentially holds the jumble order of flashcards - and
	       some negative ones.  Our current flashcard is not state.seen!
	    */
	    state.dbcurrec = state.order[state.seen] - 1;
	}    
    
    /* Query the database for the chosen record */
    dbID = DmFindDatabase(0, state.dbname);
    dbRef = NULL;
    
    if (dbID > 0)
	dbRef =  DmOpenDatabase(0, dbID, dmModeReadOnly);
    
    if (dbRef)
	{
	    h = DmQueryRecord(dbRef, state.dbcurrec);
	    StrCopy(tmp, MemHandleLock(h));
	    MemHandleUnlock(h);
	    DmCloseDatabase(dbRef);
	    
	    /* Read in the flashcard question and hooks */
	    
	    if (h)
		{
		    /*  Reset the flashcard */
		    StrCopy(flashcard, ""); 
		    
		    t = tmp;        /* set input pointer to the string */
		    s = flashcard;  /* set the output pointer to the flashcard */
		    
		    /* -- COLLINS update -- */

		    /* we could check that each character is alphanumeric but that would probably be overkill
		       afterall that is checked for when creating the databases in the first place.  Probably.
		       This loose checking allows new Collins words - annotated with "+" - to be shown. */

		    /* It would be nice to add a Preference that can switch on/off
		       the displaying of "+" characters */

		    /* Parse the flashcard question (alphagram) text from the buffer */ 
		    while (*t && *t != 9)
			*(s++) = *(t++);
		    *s = '\0';
		    
		    /* Advance the input pointer beyond the first tab character */
		    t++;

		    /* get the ANSWERS and hooks */
		    while (*t)
			{
			    s = flash.words[d];  /* reset the output pointer */
			    while (*t && *t != 47 && *t != 32) /* exists and is not SPACE or '/' */
				*(s++) = *(t++);
			    *s = '\0';  /* append terminating NULL */
			    
			    if (*t == 47) /* found a '/' */
				{
				    /* get the FRONT hooks */
				    s = flash.front[d];
				    t++;
				    while (*t != 47)
					*(s++) = *(t++) +32;    /* May 3, 2007 - Added +32 to convert hooks to lowercase */
				    *s = '\0';
				    
				    /* get the BACK hooks */
				    s = flash.back[d];
				    t++;
				    while (*t != 47)
					*(s++) = *(t++) +32;    /* May 3, 2007 - Added +32 to convert hooks to lowercase */
				    *s = '\0';
				    
				    t++;    /* advance the input pointer */
				    d++;    /* increase the ANSWERS count */
				}
			    else
				{
				    /* reset the front and back hooks */
				    StrCopy(flash.front[d], "\0");
				    StrCopy(flash.back[d], "\0");

				    if (*t == 32)
					t++;
				    d++;    /* increase the ANSWERS count */
				}
			}
		    
		}
	    else
		{
		    /* Database handle not defined */ 
		    return;
		}
	}
    else
	{
	    /* No dbRef for the database */
	}
    
    /* Reset the Bookkeeping variables */
    flash.count = d;
    revealed = 0;
    clues = 0;

    highlight = -1;


    /* save the flashcard as the alphagram */
    StrCopy(alphagram, flashcard);
}



/* SetFlashNumber() 
   - Set the flashcard counter indicator on the main form
 */
static void SetFlashNumber(UInt16 val)
{
    static Char str[COUNTFIELDSIZE + 1];
    /* If the tmp string buffer isn't being resized why not make it static? */
    Char tmp[COUNTFIELDSIZE + 1];

    StrIToA(tmp, val);
    StrCopy(str, tmp);
    StrCat(str, "/");
    StrIToA(tmp, state.visible);
    StrCat(str, tmp);

    /* Draw the counter */
    SetField(pMainFlashCounter, str, COUNTFIELDSIZE + 1);
}


/* SetCounter()
   - Set the number of answers display on the main form
*/   
static void SetCounter(UInt16 c)
{
    static Char str[COUNTFIELDSIZE + 1]; 
    
    if (prefs.showcount)
	{
	    StrIToA(str, c);
	}
    else      /* Blank the counter */
	StrCopy(str, "");

    /* Display the field */
    SetField(pMainAnsCounter, str, COUNTFIELDSIZE + 1);
}




/*
 * InitMainForm()
 *
 * Parameters: None 
 * Returns:    Nothing
 */
static void InitMainForm()
{
    /* Initialise MainForm controls */
    SetCounter(flash.count);
}



/* 
 * DBDeleteFormEventHandler()
 */
Boolean DBDeleteFormEventHandler(EventPtr event)
{
    Boolean handled = false;
    static FormPtr p;
    static Char tmpdbname[20];
    
    switch (event->eType)
	{
	case frmOpenEvent:
	    p = pCurForm;
	    pCurForm = FrmGetActiveForm();
	    
	    StrCopy(tmpdbname, state.dbname);
	    FldSetTextPtr(pDBDeleteName, tmpdbname);
	    
	    FrmDrawForm(pCurForm);
	    handled = true;
	    break;

	case frmCloseEvent:
	    StrCopy(nextDBhighlight, "\0");
	    FrmEraseForm(pCurForm);
	    FrmDeleteForm(pCurForm);
	    pCurForm = p;
	    handled = true;
	    break;
	    
	    /* Delete the DB */
	case ctlSelectEvent:
	    if (event->data.ctlSelect.controlID == DBDeleteYes)
		{
		    pCurForm = p;     

		    DeleteDB(state.dbname);

		    StrCopy(state.dbname, nextDBhighlight);     /* set next menu selection */

		    FindAllWordDBs();
		    FrmReturnToForm(0);


		    ShowWordDBs();
		    
		    handled = true;
		    break;
		}
	    if (event->data.ctlSelect.controlID == DBDeleteNo)
		{
		    FrmReturnToForm(0);
		    handled = true;
		    break;
		}
	    break;
	default:
	    break;
	}

    return handled;
}






/*
 * PrefsFormEventHandler()
 *
 * Parameters: event - pointer to the event's data
 * Returns:    True if we handled the event, otherwise false
 */
Boolean PrefsFormEventHandler(EventPtr event)
{
    Boolean handled = false;  /* Did we handle the event? */
    static FormPtr p;         /* Store the form that launched prefs */
    UInt8 changedTiles = 0;

    switch (event->eType)
	{
	case frmOpenEvent: 
	    p = pCurForm;
	    pCurForm = FrmGetActiveForm();

	    /* Ensure that the current value is highlighted when we click the
	       trigger and that the value is visible.  Do the same for the
	       other preferences below */

	    /* Update counter dropdown settings */
	    LstSetSelection(pPrefsShowCounterList, prefs.showcount);
	    LstMakeItemVisible(pPrefsShowCounterList, prefs.showcount);
	    StrCopy(countertxt, counteropts[prefs.showcount]);
	    CtlSetLabel(pPrefsShowCounterTrig, countertxt);

	    LstSetSelection(pPrefsHooksList, prefs.showhooks);
	    LstMakeItemVisible(pPrefsHooksList, prefs.showhooks);
	    StrCopy(hookstxt, counteropts[prefs.showhooks]);
	    CtlSetLabel(pPrefsHooksTrig, hookstxt);

	    LstSetSelection(pPrefsShowTilesList, prefs.showtiles);
	    LstMakeItemVisible(pPrefsShowTilesList, prefs.showtiles);
	    StrCopy(showtilestxt, counteropts[prefs.showtiles]);
	    CtlSetLabel(pPrefsShowTilesTrig, showtilestxt);

	    /* Display the form */
	    FrmDrawForm(pCurForm);
	    handled = true;
	    break;
	    
	case frmCloseEvent: /* Close the form */
	    FrmEraseForm(pCurForm);
	    FrmDeleteForm(pCurForm);
	    pCurForm = p;
	    handled = true;
	    break;
	    
	    /* Handle the form buttons */
	case ctlSelectEvent:
	    if (event->data.ctlSelect.controlID == PrefsOKButton)
		{
		    if (prefs.showtiles != LstGetSelection(pPrefsShowTilesList))
			changedTiles = 1;

		    prefs.showtiles = LstGetSelection(pPrefsShowTilesList);
		    prefs.showcount = LstGetSelection(pPrefsShowCounterList);
		    prefs.showhooks = LstGetSelection(pPrefsHooksList);


		    pCurForm = p;
		    FrmReturnToForm(0);
		    
		    if (state.visible > 0)
			{
			    ShowAnswers(revealed, clues);
			    if (changedTiles == 1) ClearFlashField();
			    SetFlashField();
			    SetCounter(flash.count);
			}

		    handled = true;
		    break;
		    
		}
	    if (event->data.ctlSelect.controlID == PrefsCancelButton)
		{
		    pCurForm = p;
		    FrmReturnToForm(0);
		    handled = true;
		    break;
		}
	    break;
	default:
	    break;
	    
	}
    
    return handled;
}


/*
 * Dictionary form event handler
 */
Boolean DictFormEventHandler(EventPtr event)
{
    Boolean handled = false;  /* Did we handle the event? */
    static FormPtr p;         /* Store the form that launched prefs */
    Char *textptr, *s, *t, tmp[MAXDBTITLE+1];
    UInt16 hstart, hend;      /* start and end points of highlighted text */
    

    switch (event->eType)
      {
      case frmOpenEvent: 
	p = pCurForm;
	pCurForm = FrmGetActiveForm();
	
	/* Display the form */
	FrmDrawForm(pCurForm);

	SetField(pDictInputField, lookup, MAXDBTITLE+1);
	LookupDefinition(lookup);
	
	handled = true;
	break;
	
      case frmCloseEvent: 
	FrmEraseForm(pCurForm);
	FrmDeleteForm(pCurForm);
	pCurForm = p;
	
	handled = true;
	break;
	
	
	/* Handle buttons on the form */

      case ctlSelectEvent:
	if (event->data.ctlSelect.controlID == DictButtonSearch)
	  {
	    FldGetSelection(pDictTextField, &hstart, &hend);
	    if ((hend-hstart) > 0) {
	      /* Dictionary has only words upto 8 letters long... */
	      if (((hend-hstart) > 1 ) && ((hend - hstart) < 9)) {
		textptr = FldGetTextPtr(pDictTextField);
		s = tmp;
		for (t = textptr + hstart; t < (textptr + hend); t++) {
		  *s++ = *t;
		}
		*s = '\0';
		textptr = NULL;
		StrCopy(lookup, tmp);
		CleanUpString(lookup);
		
		/* As we are jumping to the word it needs to be written into the input field. */
		SetField(pDictInputField, lookup, MAXDBTITLE+1);
		LookupDefinition(lookup);
	      }
	      else {
		FrmAlert(WordTooLong);
	      }
	    }
	    else {
	      /* check the string is not null */
	      /* clean up the string */
	      /* do search */
	      textptr = FldGetTextPtr(pDictInputField);
	      if (textptr != NULL) {
		StrCopy(lookup, textptr);
		textptr = NULL;
	      }
	      /* cleanup string */
	      CleanUpString(lookup);
	      LookupDefinition(lookup);
	    }
	    handled = true;
	    break;
	    
	  }
	if (event->data.ctlSelect.controlID == DictButtonNext)
	  {
	    handled = true;
	    break;
	  }
	if (event->data.ctlSelect.controlID == DictButtonPrev)
	  {
	    handled = true;
	    break;
	  }

	if (event->data.ctlSelect.controlID == DictButtonDone)
	  {
	    pCurForm = p;
	    FrmReturnToForm(0);
	    /* Unselect the selected word in the list. */
	    LstSetSelection(pMainWordList, noListSelection);
	    handled = true;
	    break;
	  }

	/* Jump button removed as the Find button works just as well with 
	   dual purpose.  If text is highlighted in the Multi-line field 
	   then it is used for lookup, otherwise the input field is used. */
	/* Leaving the code here for reference, but this is probably identical
	   to the block in the DictButtonSearch section. */



	break;
      default:
	break;
	
      }
    
    return handled;
}




/*
 * DBFormEventHandler()
 *
 * Parameters: event - pointer to the event's data
 * Returns:    True if we handled the event, otherwise false
 */
Boolean DBFormEventHandler(EventPtr event)
{
    Boolean handled = false;     /* Did we handle the event? */
    static FormPtr        p;     /* Save the form that launched dbform */
    WinDirectionType    dir;     /* Scrollbar direction */
    Int16               val;     /* Repeat value */
    UInt16       dbSelected;     /* Identifier of selected DB */
    UInt16              top;
    Char    tmp[MAXDBTITLE];     /* tmp buffer for deleted DB title */

    /* Counter for the number of databases to be displayed.  Initially memory
       is allocated for INITNODBS databases - as more are found this is
       increased in steps of 5 */ 

    dblistsize = INITNODBS;

    switch (event->eType)
	{
	case frmOpenEvent: /* Open the form */
	    pCurForm = FrmGetActiveForm();
	    
	    /* Allocate some memory for the database titles - if none already exists */

	    /* Allocate initial block of memory for INITNODBS
	       for the database titles AND the title pointers */ 

	    /* Find and display the available databases */
	    FindAllWordDBs();

	    FrmDrawForm(pCurForm);

	    ShowWordDBs();

	    handled = true;
	    break;
	    
	case ctlSelectEvent:
	    if (event->data.ctlSelect.controlID == DBButtonOK) // now labeled "Play"
		{
		    if (dbc > 0)
			{
			    /* Check that a word list has been selected */
			    dbSelected = LstGetSelection(pDBList);
			    handled = true;
			    
			    if (dbSelected == noListSelection) 
				{
				    handled = true;
				    break;
				}
			    else
				{
				    /* Store the database name */
				    if (pdb != NULL)
					{
					    StrCopy(state.dbname, pdb[dbSelected]->title);
					}				    
				    /* Free the memory pointers db and pdb */
				    CleanUpDBPointers(); 
				    
				    /* Unset the variable indicating that the last quiz was deleted  - we have a quiz */
				    state.noCurrDB = 0;

				    /* Open the MainForm */
				    FrmGotoForm(MainForm);

				    restore = false;
				    handled = true;
				}
			}
		    else {
			FrmAlert(NoDatabases);
			handled = true;
			break;
		    }
		}
	    break;


	case keyDownEvent:
	  if (IsFiveWayNavEvent(event)) 
	    {
	      /* Handle five-way navigation buttons on Tungsten E and Z31 devices (+others) */
	      if (NavSelectPressed(event))
		{
		  if (dbc > 0) 
		    {
		      /* Check that a word list has been selected */
		      dbSelected = LstGetSelection(pDBList);
		      handled = true;
		      
		      if (dbSelected == noListSelection) 
			handled = true;
		      else
			{
			  /* Store the database name */
			  if (pdb != NULL)
			    {
			      StrCopy(state.dbname, pdb[dbSelected]->title);
			    }					    
			  /* Free the memory pointers db and pdb */
			  CleanUpDBPointers(); 
			  
			  /* Unset the variable indicating that the last quiz was deleted  - we have a quiz */
			  state.noCurrDB = 0;
			  
			  
			  /* Open the MainForm */
			  FrmGotoForm(MainForm);
			  
			  restore = false;
			  handled = true;
			  
			}
		    }
		  else
		    {
		      FrmAlert(NoDatabases);
		      handled = true;
		      break;
		    }
		  break;
		}
	      /* Press DOWN key */
	      if (NavDirectionPressed(event, Down))
		{
		  if (dbc > 0)
		    WordDBScroll(DOWN);
		  handled = true;
		  break;
		}
	      /* Press UP key */
	      if (NavDirectionPressed(event, Up))
		{
		  if (dbc > 0)
		    WordDBScroll(UP);
		  handled = true;
		  break;
		}

	      /* PALM TX */

	      /* Handle the 5-way down button on TX */
	      if(event->data.keyDown.chr == 0x00c)
		{
		  if (dbc > 0)
		    WordDBScroll(DOWN);
		  handled = true;
		  break;
		}			    
	      
	      /* Handle the 5-way up button on TX */
	      if (event->data.keyDown.chr == 0x00b)
		{
		  if (dbc > 0)
		    WordDBScroll(UP);
		  handled = true;
		  break;
		}


	      /* Wootz */
	      break;
	    }
	  else
	    {
	      /* Not a FiveWayNavEvent() */

	      if (event->data.keyDown.chr == vchrPageUp)
		{
		  if (dbc > 0)
		    WordDBScroll(UP);
		  handled = true;
		  break;
		}
	      if (event->data.keyDown.chr == vchrPageDown)
		{
		  if (dbc > 0)
		    WordDBScroll(DOWN);
		  handled = true;
		  break;
		}

	      /* Handle the 5-way UP button on Tungsten E */
	      if (event->data.keyDown.chr == chrUpArrow)
		{
		  if (dbc > 0)
		    WordDBScroll(UP);
		  handled = true;
		  break;
		}
	      /* Handle the 5-way DOWN button on Tungsten E */
	      if (event->data.keyDown.chr == chrDownArrow)
		{
		  if (dbc > 0)
		    WordDBScroll(DOWN);
		  handled = true;
		  break;
		}

	      
	      /* Handle the 5-way Select button on TX */
	      if (event->data.keyDown.chr == 0x13d)
		{
		  if (dbc > 0) 
		    {
		      dbSelected = LstGetSelection(pDBList);
		      if (dbSelected == noListSelection) 
			{
			  handled = true;
			  break;
			}
		      else
			{
			  if (pdb != NULL)
			    StrCopy(state.dbname, pdb[dbSelected]->title);
			  CleanUpDBPointers(); 
			  state.noCurrDB = 0;
			  FrmGotoForm(MainForm);
			  restore = false;
			  handled = true;
			  break;
			}
		    }
		  else
		    {
		      FrmAlert(NoDatabases);
		      handled = true;
		      break;
		    }
		  break;
		}

	    }
	  
	  /* Delete DB selected from the DB menu */
	case menuEvent:
	  if (dbc == 0)
	    {
	      handled = true;
	      break;
	    }
	  switch(event->data.menu.itemID)
	    {
	    case DBMenuOptsDelete:
	      if (event->data.ctlSelect.controlID == DBMenuOptsDelete) //overkill?
		{
		  /* Get the highlighted DB title from the list */
		  dbSelected = LstGetSelection(pDBList);
		  if (dbSelected != noListSelection)
		    {
		      if (pdb)
			{
			  
			  /* Compare the last quiz with the one to be deleted - if they
			     match then we must set the state.noCurrDB = 1 to prevent restoring
			     a deleted quiz */
			  
			  StrCopy(state.dbname, pdb[dbSelected]->title);
			  
			  /* If we are deleting ANY quiz then set the no current DB flag to 1 */
			  state.noCurrDB = 1;
			  
			  
			  /* If we are deleting the last quiz played then we don't try to restore it */
			  //					    if (StrCompare(tmp, state.dbname) == 0) {
			  //						state.noCurrDB = 1;
			  //					    }
			  
			  if (dbc > 1) {
			    
			    /* Set the highlight bar for when we return to the DB form */
			    if (dbSelected + 1 < dbc) {
			      StrCopy(nextDBhighlight, pdb[dbSelected + 1]->title);
			    } 
			    else {
			      StrCopy(nextDBhighlight, pdb[dbSelected - 1]->title); 
			    }
			  }
			  else {
			    StrCopy(nextDBhighlight, "\0");
			  }
			  
			  /* 
			     Popup the question - "Delete or Cancel?"
			  */
			  
			  FrmPopupForm(DBDeleteForm);
			}
		    }
		  handled = true;
		  break;
		}
	      
	      
	      break;
	      
	      /* Beam DB selected from the DB menu */
	    case DBMenuOptsBeam:
	      dbSelected = LstGetSelection(pDBList);
	      
	      
	      /* Check if a DB title is selected */ 
	      if (dbSelected == noListSelection)  
		{
		  handled = true;
		  break;
		}
	      
	      /* Copy the DB title into the global current DB name buffer */
	      if (pdb)  
		{
		  StrCopy(state.dbname, pdb[dbSelected]->title);
		  SendMe();  // beam the DB
		}
	      
	      handled = true;
	      break;
	    }
	  handled = true;
	  break;
	  
	  
	case lstEnterEvent:
	  if (dbc == 0)
	    {
	      handled = true;
	      break;
	    }
	  if (event->data.lstEnter.pList == pDBList)
	    {
	      LstHandleEvent(pDBList, event);
	      top = LstGetTopItem(pDBList);
	      
	      if (LstGetNumberOfItems(pDBList) > DBLISTMAX)
		{
		  /* set scrollbar */
		  SclSetScrollBar(pDBListScroll, top, 0, dbc <= DBLISTMAX ? 0 : dbc - DBLISTMAX, DBLISTMAX);
		}
	      handled = true;
	    }
	  break;
	  
	  

	  
	  /* Scroll bar repeat. */
	case sclRepeatEvent:
	  if (dbc == 0) {
	    handled = true;
	    break;
	  }
	  val = event->data.sclRepeat.newValue -
	    event->data.sclRepeat.value;
	  dir = winDown;
	  if (val < 0)
	    {
	      dir = winUp;
	      val = -val;
	    }
	  LstScrollList(pDBList, dir, val);
	  break;
	  
	default:
	  break;
	  
	}
    
    return handled;
}



/*
 * MainFormEventHandler()
 *
 * Parameters: event - pointer to the event's data
 * Returns:    true if we handled the event, false if we didn't  
 */
Boolean MainFormEventHandler(EventPtr event)
{
    
#define INFLASHCARD(x) ( ((x < YOFFSET+TILEHEIGHT) && (x > YOFFSET)) )


    Boolean handled = false;     /* Did we handle the event? */
    UInt16            error;    
    WinDirectionType    dir;     /* Scrollbar direction */
    Int16               val;     /* Repeat value */
    Err                 err;     /* Error code returned by CountRecordsInDB() - if nonzero then DB open failed */
    UInt8 i, j;  /* counter over tiles within the flashcard */
    Char foo;
    //
    Int16 tmpwordid;
    Char tmpword[MAXDBTITLE+1];
    Char *t; Char *s;

    UInt16 barf;

    switch (event->eType)
	{
	case frmOpenEvent: 
	    pCurForm = FrmGetActiveForm();

	    /* If the lampflash.data DB does not exist then create it. */
	    err = CountRecordsInDB();

	    if (err == 0)
		{
		    /* When restore is true we are restarting the quiz after 
		       powering-on, so the state variables are already set.
		    */
		    
		    if (!restore)
			ResetStats();
		    
		    if (state.visible > 0)
			{
			    GetNewFlashcard();

			    			    
			    /* Reset the restore variable and continue */
			    if (restore)
				restore = false;
			    
			    
			    /* Are we doing anagrams or hooks? */
			    if (StrLen(flashcard) == StrLen(flash.words[0]))
				doingHooks = false;
			    else
				doingHooks = true;
			    
			    
			    /* Order the flashcard according to the prefs.letterorder */
			    /* Alphagram, Random or Vow-Consonant */
			    OrderNewFlashcard();
			    
			    SetWordOrder();

			    
			    /* Set Title */
			    FrmDrawForm(pCurForm);			    
			    ShowAnswers(NONE, 0);

			    FldSetTextPtr(pMainDBTitle, state.dbname);
			    FldDrawField(pMainDBTitle);

			    SetFlashNumber(state.curr);
			    SetCounter(flash.count);


			    /* Finally, draw the form... */
			    /* We cannot draw the flashcards by WinDrawChars until
			       the form has been drawn */

			    SetUpFlashcardField();
			    SetFlashField();

			}
		    else
			{
			    /* Draw the Blank form */
			    DrawBlankForm();
			}
			    
		}		    
	    else
		{
		    FrmCloseAllForms();
		    FrmGotoForm(DBForm);
		}

	    handled = true;
	    break;

	    /* Dictionary lookup calls */	    
	case lstEnterEvent:
	    if (event->data.lstEnter.pList == pMainWordList)
		{
		  /* If the list is entered allow the built-in handler deal with highlighting a word. */
		  LstHandleEvent(pMainWordList, event);

		  tmpwordid = LstGetSelection(pMainWordList);
		  /* If a word IS selected then look it up in the dictionary. */
		  if (tmpwordid != noListSelection)
		    {
		      /* Buffer the selected word and copy it to lookup[]. */
		      StrCopy(tmpword, display[tmpwordid]);
		      
		      for (i = 0; i < StrLen(tmpword); i++) {
			if ( ((tmpword[i] > 64) && (tmpword[i] < 91)) ||
			     ((tmpword[i] > 96) && (tmpword[i] < 123)) ) {
			}
			else {
			  tmpword[i] = '\0';
			  break;
			}
		      }

		      /* Convert the lookup string to lowercase */
		      StrToLower(lookup, tmpword);
		      
		      FrmPopupForm(DictForm);
		    }

		  handled = true;
		}
	    break;

	case penDownEvent:
	    {
		if (prefs.showtiles != 0)
		    {
			if (INFLASHCARD(event->screenY))
			    {
				/* This could be reworked for more efficiency */

				for (i = 0; i < StrLen(flashcard); i++)
				    {
					if ((event->screenX > rack[i].topLeft.x) && 
					    (event->screenX < rack[i].topLeft.x + rack[i].extent.x))
					    {
						/* Nothing highlighted */
						if (highlight < 0)
						    {
							DrawTile(rack[i], flashcard[i], 1);
							highlight = i;
							handled = true;
							break;
						    }
						else
						    {
							if (highlight == i) 
							    {
								highlight = -1;
								DrawTile(rack[i], flashcard[i], 0);
								handled = true;
								break;
							    }

							/* otherwise swap */
							foo = flashcard[highlight];
							if (highlight > i)
							    {
								for (j = highlight; j > i; j--)
								    {
									flashcard[j] = flashcard[j-1];
									DrawTile(rack[j], flashcard[j], 0);
								    }
								flashcard[i] = foo;
								DrawTile(rack[i], flashcard[i], 0);
								highlight = -1;
								handled = true;
							    }
							else
							    {
								for (j = highlight; j < i; j++)
								    {
									flashcard[j] = flashcard[j+1];
									DrawTile(rack[j], flashcard[j], 0);
								    }
								flashcard[i] = foo;
								DrawTile(rack[i], flashcard[i], 0);
								highlight = -1;
								handled = true;
								break;
							    }

						    }

					    }
				    }
				break;
			    }
			break;
		    }
		break;
	    }



	case keyDownEvent:
	    if (IsFiveWayNavEvent(event)) 
		{

		    /* DEBUG codes
		    if (event->data.keyDown.chr != 0)
			{
			    fooWrite( (UInt16) event->data.keyDown.chr, 20);
			    fooWrite( (UInt16) 0x077, 30);
			    handled = true;
			}
		    */

		    /* Add control for the rocker switch on the Z22 here */
		    if (NavSelectPressed(event))
			{
			    flashcardShowAll();
			    handled = true;
			    break;
			}
		    if (NavDirectionPressed(event, Right))
			{
			    flashcardDoNext();
			    handled = true;
			    break;
			}
		    if (NavDirectionPressed(event, Left))
			{
			    flashcardDoLast();
			    handled = true;
			    break;
			}
		    if (NavDirectionPressed(event, Down))
			{
			    flashcardShowOne();
			    handled = true;
			    break;
			}
		    if (NavDirectionPressed(event, Up))
			{
			    flashcardShowClue();
			    handled = true;
			    break;
			}



		    /* Wootz */

		    /* Handle the 5-way down button on TX */
		    if(event->data.keyDown.chr == 0x00c)
			{
			    flashcardShowOne();
			    handled = true;
			    break;
			}			    

		    /* Handle the 5-way up button on TX */
		    if (event->data.keyDown.chr == 0x00b)
			{
			    flashcardShowClue();
			    handled = true;
			    break;
			}

		}
	    else
		{

		    /* Controllers for the TX 5-way rocker - only the UP and DOWN codes */

		    /* 
		    if (event->data.keyDown.chr != 0)
			{
			    fooWrite((UInt16) event->data.keyDown.chr, 20);
			    handled = true;
			    break;
			}
		    */


		    /* Handle the 5-way RIGHT button on TX */
		    if (event->data.keyDown.chr == chrRightArrow)
			{
			    flashcardDoNext();
			    handled = true;
			    break;
			}
		    //if (event->data.keyDown.chr == 0x135)
		    if (event->data.keyDown.chr == vchrRockerRight)
			{
			    flashcardDoNext();
			    handled = true;
			    break;
			}

		    /* Handle the 5-way LEFT button on TX */
		    if (event->data.keyDown.chr == chrLeftArrow)
			{
			    flashcardDoLast();
			    handled = true;
			    break;
			}
		    //if (event->data.keyDown.chr == 0x134)
		    if (event->data.keyDown.chr == vchrRockerLeft)
			{
			    flashcardDoLast();
			    handled = true;
			    break;
			}

		    /* Handle the 5-way UP button on Tungsten E */
		    if (event->data.keyDown.chr == chrUpArrow)
			{
			    //flashcardDelete();
			    flashcardShowClue();
			    handled = true;
			    break;
			}
		    /* Handle the 5-way DOWN button on Tungsten E */
		    if (event->data.keyDown.chr == chrDownArrow)
			{
			    flashcardShowOne();
			    handled = true;
			    break;
			}



		    /* Handle the 5-way Select button on TX */
		    //if (event->data.keyDown.chr == 0x13d)
		    //if (event->data.keyDown.chr == vchrRockerCenter)
		    if (event->data.keyDown.chr == 0x13d)
			{
			    flashcardShowAll();
			    handled = true;
			    break;
			}
		    /* Think Select is unhandled on the Tungsten in the simulator */


		}
	    break;

    
	case ctlSelectEvent: /* Handle buttons */
	    /* pppp */

	    if (event->data.ctlSelect.controlID == OrderRandPush)
		{
		    if (state.visible > 0)
			{
			    prefs.letterorder = 1;
			    highlight = -1;
			    RandomiseFlashcard();
			    SetFlashField();
			}
		}
	    if (event->data.ctlSelect.controlID == OrderAlphaPush)
		{
		    if (state.visible > 0)
			{
			    prefs.letterorder = 0;
			    highlight = -1;
			    AlphagramiseFlashcard();
			    SetFlashField();
			}
		}
	    if (event->data.ctlSelect.controlID == OrderVowconPush)
		{
		    if (state.visible > 0)
			{
			    prefs.letterorder = 2;
			    highlight = -1;
			    VowConiseFlashcard();
			    SetFlashField();
			}
		}


	    /* Show Next flashcard */
	    if (event->data.ctlSelect.controlID == MainNext)
		{
		    flashcardDoNext();
		    handled = true;
		    break;
		}

	    /* Show Last flashcard */
	    if (event->data.ctlSelect.controlID == MainLast)
		{
		    flashcardDoLast();
		    handled = true;
		    break;
		}

	    /* Show Answers */
	    if (event->data.ctlSelect.controlID == MainAnswers)
		{
		    flashcardShowAll();
		    handled = true;
		    break;
		}

	    /* Show Clue to Next Answer */
	    if (event->data.ctlSelect.controlID == MainClue)
		{
		    flashcardShowClue();
		    handled = true;
		    break;
		}
	    
	    /* Show Next Answer */
	    if (event->data.ctlSelect.controlID == MainNextAnswer)
		{
		    flashcardShowOne();
		    handled = true;
		    break;
		}
	    
	    /* Delete flashcard */
	    if (event->data.ctlSelect.controlID == MainDelete)
		{
		    flashcardDelete();
		    handled = true;
		    break;
		}



	    break;
	    
	    
	    /* Scroll bar repeat. */
	case sclRepeatEvent:
	    val = event->data.sclRepeat.newValue -
		event->data.sclRepeat.value;
	    dir = winDown;
	    if (val < 0)
		{
		    dir = winUp;
		    val = -val;
		}
	    LstScrollList(pMainWordList, dir, val);
	    break;
	    
	    
	    
	    
	    /* Menu event */
	case menuEvent:
	    switch(event->data.menu.itemID)
		{

		case HelpMenuNewDB:
		    /* Save lampflash.data before exiting form */
		    SaveOrderData();
		    FrmGotoForm(DBForm); 
		    break;


		case HelpMenuUndelete:
		    UndeleteAll();
		    break;
		    
		case HelpMenuShuffle:
		    {
			if (state.visible > 1)
			    DoShuffle();
			handled = true;
			break;
		    }


		case HelpMenuInst:
		    FrmHelp(InstStr);
		    break;
		    
		case HelpMenuAbout:
		    FrmCustomAlert(AboutAlert, version, NULL, NULL);
		    break;

		case HelpMenuPrefs:
		    FrmPopupForm(PrefsForm);
		    break;

		}
	    handled = true;
	    break;
	    
	    
	case appStopEvent:
	    FrmCloseAllForms();
	    
	    
	default:
	    break;

	}
    return handled;
}

/* 
 * AppEventHandler()
 *
 * Parameters: event - pointer to the event's data
 * Returns:    True if we handled the event, false if we didn't
 */
static Boolean AppEventHandler(EventPtr event)
{
    Boolean handled = false;     /* Did we handle the event? */
    FormPtr form;                /* Pointer used when loading a form */
    
    switch (event->eType)
	{
	case frmLoadEvent:
	    form = FrmInitForm(event->data.frmLoad.formID);
	    FrmSetActiveForm(form);

	    /* 
	       Define global pointers to each control and field on the
	       form.  This will save time later.  Borrowed from LAMPWords.
	    */
	    
	    if (event->data.frmLoad.formID == MainForm)
		{
		    /* Declare the MainForm control and field pointers */
		    pMainFlashCounter = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, MainFlashCounter));
		    pMainAnsCounter = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, MainAnsCounter));
		    pMainDBTitle = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, MainDBTitle));
		    pMainWordList = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, MainWordList));
		    pMainScrollBar = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, MainWordScroll));
		    pMainAnswers = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, MainAnswers));
		    pMainNextAnswer = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, MainNextAnswer));
		    pMainDelete = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, MainDelete));
		    pMainClue = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, MainClue));
		    pMainNext = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, MainNext));
		    pMainLast = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, MainLast));
		    pMainOrderAlphaPush = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, OrderAlphaPush));
		    pMainOrderRandPush = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, OrderRandPush));
		    pMainOrderVowconPush = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, OrderVowconPush));

		    /* Declare the event handler */
		    FrmSetEventHandler(form, MainFormEventHandler);
		}

	    /* Dictionary form controls */ 
	    if (event->data.frmLoad.formID == DictForm)
	      {
		pDictTextField = 
		  FrmGetObjectPtr(form, FrmGetObjectIndex(form, DictTextField));
		pDictInputField = 
		  FrmGetObjectPtr(form, FrmGetObjectIndex(form, DictInputField));
		pDictWordField = 
		  FrmGetObjectPtr(form, FrmGetObjectIndex(form, DictWordField));

		FrmSetEventHandler(form, DictFormEventHandler);
	      }


	    /* Preferences form controls */
	    if (event->data.frmLoad.formID == PrefsForm)
		{
		    pPrefsShowCounterTrig = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, ShowCounterTrig));
		    pPrefsShowCounterList = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, ShowCounterList));
		    pPrefsHooksTrig = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, HooksTrig));
		    pPrefsHooksList = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, HooksList));
		    pPrefsShowTilesTrig = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, ShowTilesTrig));
		    pPrefsShowTilesList = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, ShowTilesList));
		    
		    /* Declare the event handler */
		    FrmSetEventHandler(form, PrefsFormEventHandler);
		}
	    
	    
	    if (event->data.frmLoad.formID == DBForm)
		{
		    /* Declare the DBForm control and field pointers */
		    pDBList = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, DBList));
		    pDBButtonOK = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, DBButtonOK));
		    pDBListScroll = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, DBListScroll));
		    
		    /* Declare the event handler */
		    FrmSetEventHandler(form, DBFormEventHandler);
		}
	    

	    if (event->data.frmLoad.formID == DBDeleteForm)
		{
		    /* Declare the DBForm control and field pointers */
		    pDBDeleteYes = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, DBDeleteYes));
		    pDBDeleteNo = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, DBDeleteNo));
		    pDBDeleteName = 
			FrmGetObjectPtr(form, FrmGetObjectIndex(form, DBDeleteName));
		    
		    /* Declare the event handler */
		    FrmSetEventHandler(form, DBDeleteFormEventHandler);
		}
	    

	    handled = true;
	    break;
	    
	default:
	    break;
	}
    
    return handled;
}



/* 
 * StopApplication()
 *
 * Parameters: None
 * Returns:    Nothing
 */
static void StopApplication(void)
{
    /* Perform deinitialisation when the program ends */
    if (dbh)
	{
	    MemHandleUnlock(dbh);  
	    MemHandleFree(dbh);
	}
    if (pdbh)
	{
	    MemHandleUnlock(pdbh);
	    MemHandleFree(pdbh);
	}
    if (ppah)
	{
	    MemHandleUnlock(ppah);
	    MemHandleFree(ppah);
	}
    
    PrefSetAppPreferences(CREATORID, PREFSID, PREFSVERSION, &prefs, sizeof(prefsType), true);
    PrefSetAppPreferences(CREATORID, STATEID, STATEVERSION, &state, sizeof(stateType), false);

    FrmCloseAllForms();

    /* No databases to close... I think */
}




      
/*
 * StartApplication()
 *
 * Parameters: None
 * Returns:    Error code
 */
static Err StartApplication(void)
{
    UInt16         size;  /* size of various data structures */
    Int16           res;  /* result code */

    /* Is there at least one DB? */ 
    Err                         err;
    DmSearchStateType   searchState;
    UInt32                     type;
    UInt16                   cardNo;
    LocalID                    dbID;

    /* Copy the version string from VersionStr defined in .rcp file */
    SysCopyStringResource(version, VersionStr);
    

    /* -----------------------------
       Read and restore PREFERENCES 
    */

    size = sizeof(prefsType);  
    res = PrefGetAppPreferences(CREATORID, PREFSID, &prefs, &size, true);

    /* If no system preferences can be found or if the version number of the
       preferences has changed since last used the set them to the defaults
       defined here. */

    if (res != PREFSVERSION || size != sizeof(prefsType))
	{
	    MemSet(&prefs, sizeof(prefsType), 0);
	    prefs.showcount = 1;
	    prefs.letterorder = 0;
	    prefs.showhooks = 1;
	    prefs.showtiles = 1;
	}

    /* -----------------------
       Read and restore STATE
    */
    size = sizeof(stateType);
    res = PrefGetAppPreferences(CREATORID, STATEID, &state, &size, false);


    /* If the size or version number of STATE has changed then
       create space for a new set of state variables and start from fresh */

    if (res != STATEVERSION || size != sizeof(stateType))
	{
	    /* if the state restore failed - or the version or size have
	       changed then create a new state and goto DB form */ 

	    MemSet(&state, sizeof(stateType), 0);
	    //	    StrCopy(state.dbname, "\0");
	    state.noCurrDB = 1;  /* there is no current DB */ 
	    FrmGotoForm(DBForm);
	}
    else
	{
	    restore = true;

	    if (state.noCurrDB == 1){

		/* There is no current DB so just to the DB form */
		FrmGotoForm(DBForm);

	    }
	    else {
		/* We have a last DB so jump to it */

		/* TODO: 
		   I suppose I could add a quick - "if the DB doesn't exist"
		   sanity check here and jump to the DBForm page if it
		   doesn't... */

		FrmGotoForm(MainForm);
	    }
	}

    /* Ok.  Launch the main form */
    
    return 0;
}



/*
 * EventLoop()
 *
 * Parameters: None
 * Returns:    Nothing
 */
static void EventLoop(void)
{
    /* Fetch and dispatch events to the various event handlers */

    /* Note: e is the value in the event pointer "event" used throughout */
    EventType e;   /* Buffer that holds the event data */
    UInt16 error;  /* Error code */
    
    do
	{
	    EvtGetEvent(&e, evtWaitForever);
	    
	    if (! SysHandleEvent(&e))
		if (! MenuHandleEvent(0, &e, &error))
		    if (! AppEventHandler(&e))
			FrmDispatchEvent(&e);
	} 
    while(e.eType != appStopEvent);
}

      

/*
 * PilotMain()
 *
 * Parameters: See PalmOS documentation
 * Returns:    See PalmOS documentation
 */
UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
    Err error = 0;
    if (cmd == sysAppLaunchCmdNormalLaunch) 
	{
	    error = StartApplication();     
	    if (error) return error;
	    
	    EventLoop();
	    StopApplication();
	}

    return error;
}      


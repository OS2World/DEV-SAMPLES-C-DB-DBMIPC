static unsigned char sqla_program_id[40] = 
{111,65,65,66,65,69,67,67,78,69,87,84,79,78,32,32,87,79,82,75,
69,82,32,32,108,65,103,104,85,77,69,74,48,32,32,32,32,32,32,32};


/* Operating System Control Parameters */
#ifndef SQL_API_RC
#define SQL_STRUCTURE struct
#define SQL_API_RC int
#define SQL_POINTER
#ifdef  SQL16TO32
#define SQL_API_FN far pascal _loadds
#else
#define SQL_API_FN _System
#endif
#endif

struct sqlca;
struct sqlda;
SQL_API_RC SQL_API_FN  sqlaaloc(unsigned short,
                                unsigned short,
                                unsigned short,
                                void *);
SQL_API_RC SQL_API_FN  sqlacall(unsigned short,
                                unsigned short,
                                unsigned short,
                                unsigned short,
                                void *);
SQL_API_RC SQL_API_FN  sqladloc(unsigned short,
                                void *);
SQL_API_RC SQL_API_FN  sqlasets(unsigned short,
                                void *,
                                void *);
SQL_API_RC SQL_API_FN  sqlasetv(unsigned short,
                                unsigned short,
                                unsigned short,
                                unsigned short,
                                void *,
                                void *,
                                void *);
SQL_API_RC SQL_API_FN  sqlastop(void *);
SQL_API_RC SQL_API_FN  sqlastrt(void *,
                                void *,
                                struct sqlca *);
SQL_API_RC SQL_API_FN  sqlausda(unsigned short,
                                struct sqlda *,
                                void *);
SQL_API_RC SQL_API_FN  sqlestrd_api(unsigned char *,
                                    unsigned char *,
                                    unsigned char,
                                    struct sqlca *);
SQL_API_RC SQL_API_FN  sqlestpd_api(struct sqlca *);
#line 1 "D:\\dale\\stuff\\worker.sqc"
#define INCL_DOS
#include <os2.h>
#include "master.h"
#include <sqlenv.h>
static HFILE hError=0;      /* allows for multiple writes to this file although the code doesn't do it */

/* this routine is necessary for error handling since this process runs in
   the background and is incapable of writing to stdout/stderr
   It will write to the indicated file, the string passed in
   This program only uses the database name as the file name. That file
   is placed in the current directory
*/
static void ErrorLog(char *fname,char * infl)
{
ULONG oAction;

   DosBeep( 540,300);
  if (hError==0)
  {
   if (DosOpen(fname,&hError,&oAction,0L,FILE_NORMAL,
           OPEN_ACTION_REPLACE_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW,
           OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE | OPEN_FLAGS_WRITE_THROUGH,
           0L))
           DosBeep( 540,300);
  }
  if (hError!=0)
  {
    ULONG  byteswr;
    ULONG  bytesout;
    APIRET rc;
     bytesout= strlen(infl);
     rc =DosWrite(hError,infl,bytesout,&byteswr) ;
     if (rc != 0 || bytesout != byteswr)
           DosBeep( 540,300);
   }
}
main(argc, argv, envp)
   int argc;
   char *argv[];
{
  struct sqlca sqlca;
  int myindex;                     /* index into shared memory as passed in */
  ULONG postct;                    /* for use with DosResetEventSem */
  APIRET rc;                       /* OS/2 return codes */
  char work[100];
  /* sql stuff */
  
/*
EXEC SQL BEGIN DECLARE SECTION;
*/
#line 47 "D:\\dale\\stuff\\worker.sqc"

  long key;
  long value;
  
/*
EXEC SQL END   DECLARE SECTION;
*/
#line 50 "D:\\dale\\stuff\\worker.sqc"


  HEV mysem;                     /* handle for my semaphore */
  HEV mastersem;                 /* handle for master.exe's semaphore */
  BOOL BFinished=FALSE;
  PTIB    threadptr;             /* hold thread information after DosGetInfoblocks */
  PPIB    procptr;              /* hold process information after DosGetInfoblocks */
 struct _commarea * SharedMem;  /* shared memory pointer */
 struct _commarea * MyPieceofSharedMem;  /* points within SharedMem to my stuff */

  struct _updatestuff  * stuff;    /* work pointer when I  want to use input area as */
                                   /* something other than a string */
  if (argc !=2)
  {
       ErrorLog("unknown","worker not invoked with a valid option");
       DosExit(EXIT_PROCESS,8);
  }
  /* I should be passed my index as an argument, convert to binary
     and validate */
  myindex=atoi(argv[1]);
  if (myindex > DBNAMECOUNT-1 || myindex < 0)
  {
       sprintf(work,"worker invoked with invalid option %ld",myindex);
       ErrorLog("unknown",work);
       DosExit(EXIT_PROCESS,8);

  }
  /* connect to the shared memory created by master.exe */

  rc=DosGetNamedSharedMem((void *)&SharedMem,SHARED_MEM_NAME,PAG_READ|PAG_WRITE);
  if (rc)
  {
       sprintf(work,"unable to connect to shared memory rc= %ld",rc);
       ErrorLog(dbNames[myindex],work);
       DosExit(EXIT_PROCESS,8);

  }
       DosGetInfoBlocks(&threadptr,&procptr);
  MyPieceofSharedMem =SharedMem+myindex;
  /* open event semaphore created by master process note mastersem must be zero
     on entry or "invalid parameter" will be returned */
 mastersem = 0L;
  rc=DosOpenEventSem(MASTER_SEM_NAME,&mastersem);
  if (rc)
  {
       DosFreeMem(SharedMem);
       sprintf(work,"unable to open master semaphore rc= %ld",rc);
       ErrorLog(dbNames[myindex],work);
       DosExit(EXIT_PROCESS,8);

  }
  /* create my semaphore name based on the database and then open it. It
     will have been created by master.exe */
  sprintf(work,"\\SEM32\\%s",dbNames[myindex]);
  mysem=0L;
  rc=DosOpenEventSem(work,&mysem);
  if (rc)
  {
       MyPieceofSharedMem->result=ITSBAD;
       DosPostEventSem(mastersem);
       DosCloseEventSem(mastersem);
       DosFreeMem(SharedMem);
       sprintf(work,"unable to open my semaphore rc= %ld",rc);
       ErrorLog(dbNames[myindex],work);
       DosExit(EXIT_PROCESS,8);

  }
  /* start using the database.. can use EXEC SQL CONNECT if this is DB2/2 */

  sqlestrd(dbNames[myindex],SQL_USE_SHR,&sqlca);
  if (SQLCODE != 0)
  {
       MyPieceofSharedMem->result=ITSBAD;
       DosPostEventSem(mastersem);
       DosCloseEventSem(mastersem);
       DosFreeMem(SharedMem);
       sprintf(work,"unable to connect to database rc= %ld",SQLCODE);
       ErrorLog(dbNames[myindex],work);
       DosExit(EXIT_PROCESS,8);
  }

  /* flag all is well in shared memory and post the master  */

  MyPieceofSharedMem->result=ITSOK;
  stuff = (struct _updatestuff  *)(MyPieceofSharedMem->inarea);
  rc=DosPostEventSem(mastersem);
  if (rc)
  {
       sprintf(work,"unable to post master semaphore rc= %ld",rc);
       ErrorLog(dbNames[myindex],work);
       goto ERROREXIT;
  }
  /* loop until I'm told to die */
  while (!BFinished)
  {
      /* wait for action */

      rc=DosWaitEventSem(mysem,SEM_INDEFINITE_WAIT);
      if (rc)
      {
           sprintf(work,"unable to wait on worker semaphore rc= %ld",rc);
           ErrorLog(dbNames[myindex],work);
           goto ERROREXIT;
      }
      switch (MyPieceofSharedMem->requestcode) {
       case REQTERM:
            BFinished=TRUE;
            sprintf(work,"PID %ld Terminating\n",procptr->pib_ulpid);
            strcpy(MyPieceofSharedMem->outarea,work);
            MyPieceofSharedMem->result=ITSOK;
          break;
       case REQBEEP:
            DosBeep( (myindex*200)+330,300);
            sprintf(work,"PID %ld has beeped\n",procptr->pib_ulpid);
            strcpy(MyPieceofSharedMem->outarea,work);
            MyPieceofSharedMem->result=ITSAWARNING;
          break;
       case REQUPDATE:
           key=  stuff->key;
           value= stuff->value;
           
/*
EXEC SQL UPDATE DEMOTABLE SET INFO=:value WHERE TABKEY=:key;
*/

{
  sqlastrt(sqla_program_id,0L,&sqlca);
  sqlaaloc(1,2,1,0L);
    sqlasetv(1,0,496,4,&value,0L,0L);
    sqlasetv(1,1,496,4,&key,0L,0L);
  sqlacall((unsigned short)24,1,1,0,0L);
  sqlastop(0L);
}
#line 170 "D:\\dale\\stuff\\worker.sqc"

           if (SQLCODE==0)
           {
              
/*
EXEC SQL COMMIT WORK;
*/

{
  sqlastrt(sqla_program_id,0L,&sqlca);
  sqlacall((unsigned short)21,0,0,0,0L);
  sqlastop(0L);
}
#line 173 "D:\\dale\\stuff\\worker.sqc"

              if (SQLCODE!=0)
              {
               sprintf(MyPieceofSharedMem->outarea,
                    "Commit of update failed sqlcode = %ld",SQLCODE);
               MyPieceofSharedMem->result=ITSBAD;

              }
              else
              {
               sprintf(MyPieceofSharedMem->outarea,
                 "Update of DEMOTABLE in db <%s> successful\n",dbNames[myindex]);
               MyPieceofSharedMem->result=ITSOK;
              }
           }
           else
           {
            sprintf(MyPieceofSharedMem->outarea,"Update failed sqlcode = %ld",SQLCODE);
            MyPieceofSharedMem->result=ITSBAD;
           }
          break;
       case REQINSERT:
           key=  stuff->key;
           value= stuff->value;
           
/*
EXEC SQL INSERT INTO DEMOTABLE VALUES (:key,:value);
*/

{
  sqlastrt(sqla_program_id,0L,&sqlca);
  sqlaaloc(1,2,2,0L);
    sqlasetv(1,0,496,4,&key,0L,0L);
    sqlasetv(1,1,496,4,&value,0L,0L);
  sqlacall((unsigned short)24,2,1,0,0L);
  sqlastop(0L);
}
#line 197 "D:\\dale\\stuff\\worker.sqc"

           if (SQLCODE==0)
           {
              
/*
EXEC SQL COMMIT WORK;
*/

{
  sqlastrt(sqla_program_id,0L,&sqlca);
  sqlacall((unsigned short)21,0,0,0,0L);
  sqlastop(0L);
}
#line 200 "D:\\dale\\stuff\\worker.sqc"

              if (SQLCODE!=0)
              {
               sprintf(MyPieceofSharedMem->outarea,
                    "Commit of insert failed sqlcode = %ld",SQLCODE);
               MyPieceofSharedMem->result=ITSBAD;

              }
              else
              {
               sprintf(MyPieceofSharedMem->outarea,
                 "Insert into DEMOTABLE in db <%s> successful\n",dbNames[myindex]);
               MyPieceofSharedMem->result=ITSOK;
              }
           }
           else
           {
            sprintf(MyPieceofSharedMem->outarea,"Insert failed sqlcode = %ld",SQLCODE);
            MyPieceofSharedMem->result=ITSBAD;
           }
          break;
       default:
          break;
       } /* endswitch */
      DosResetEventSem(mysem,&postct);
      rc=DosPostEventSem(mastersem);
      if (rc)
      {
           sprintf(work,"unable to post master semaphore rc= %ld",rc);
           ErrorLog(dbNames[myindex],work);
           goto ERROREXIT;
      }

  }
ERROREXIT:
       sqlestpd(&sqlca);
       DosFreeMem(SharedMem);
       DosCloseEventSem(mastersem);
       DosCloseEventSem(mysem);
       DosExit(EXIT_PROCESS,8);
}

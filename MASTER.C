#define INCL_DOS
#include <os2.h>
#include <stdio.h>
#include "master.h"               /* application defines */

main(argc, argv, envp)
   int argc;
   char *argv[];
   char *envp[];
{
 int numdbs, i,j;
 ULONG postct;                    /* used to hold reset count */
 APIRET rc;                       /* return code from OS calls */
 HEV master;                      /* My event semaphore handle*/
 char execarg[60];                /* argument string for dosexecpgm */
 char * imhere;                   /* work pointer */
 char semname[100];               /* work are to hold semaphore name */
 RESULTCODES result;              /* structure containing PID after dosexecpgm */
 int Connectcount=0;              /* Number of databases connected when count
                                     reaches zero this program will end */

 struct _commarea * SharedMem;    /* Pointer to memory shared with worker processes */

/*
 *   If argument not passed in start default number of databases, otherwise
 *   start the requested number (after validation of course)
 *
*/
 if (argc == 1)
    numdbs=DBNAMECOUNT;
 else
 {
    numdbs=atoi(argv[1]);
    if (numdbs<1 || numdbs > DBNAMECOUNT)
    {
       printf("Maximum number of databases to be started is %ld you entered %ld\n",
                DBNAMECOUNT,numdbs);
       DosExit(EXIT_PROCESS,8);
    }
 }
/*    Allocate named shared memory object of size sufficient to hold
 *    work areas for each of the databases to be started. Memory is
 *    read/write and initially committed.
 *
*/
 rc=DosAllocSharedMem((void *)&SharedMem,SHARED_MEM_NAME,numdbs*sizeof(struct _commarea),
                      PAG_COMMIT|PAG_READ|PAG_WRITE);
 if (rc)
 {
    printf("DosAllocSharedMem failed rc= %ld\n",rc);
    DosExit(EXIT_PROCESS,8);

 }
/*
 *    Create a named semaphore to be posted by workers when they
 *    are finished their work. If work is to be done asynchronously
 *    this code could be changed to handle multiple events and
 *    periodically poll for completion or MUXWAIT.
 *
*/
 rc=DosCreateEventSem(MASTER_SEM_NAME,&master,0,0);
 if (rc)
 {
    printf("DosCreateEventSem failed for Master Sem rc= %ld\n",rc);
    DosExit(EXIT_PROCESS,8);

 }
/*
 *   Build the worker semaphore names.. each name will consist of
 *   the required \SEM32\ followed by the database name. Each client
 *   will open this semaphore and wait on it until there is work to do.
 *
*/
 memcpy(semname,"\\SEM32\\",7);
 for (i=0;i<numdbs;i++)
    {
      strcpy(semname+7,dbNames[i]);
      rc=DosCreateEventSem(semname,&dbSemaphores[i],0,0);
      if (rc)
      {
         printf("DosCreateEventSem failed for %s rc= %ld\n",semname,rc);
         DosExit(EXIT_PROCESS,8);

      }
/*
 *   Start the worker via DosExecPgm, passing an index into the
 *   shared memory object where its control information will be.
 *
*/

      strcpy(execarg,CLIENT_NAME);
      j=strlen(execarg);
      j++ ;
      imhere=execarg+j;
      sprintf(imhere,"%d",i);
      imhere[strlen(imhere)+1]='\0';
      rc=DosExecPgm(NULL,0L,EXEC_BACKGROUND,execarg,NULL,&result,CLIENT_NAME);
      if (rc)
      {
         printf("DosExecProgram failed for %s rc= %ld\n",dbNames[i],rc);
         DosExit(EXIT_PROCESS,8);

      }
/*
 *     Wait indefinitely for client to startup.. you might want to
 *     change this code to wait with a timer and then check if the
 *     PID is still valid, since it is possible for worker to die
 *     because it can't open the semaphore or get memory or the like
 *
*/
      rc=DosWaitEventSem(master,SEM_INDEFINITE_WAIT);
      if (rc)
      {
         printf("DosWaitEventSem failed for Master Sem rc= %ld\n",rc);
         DosExit(EXIT_PROCESS,8);

      }
/*
 *  Reset the semaphore to clear the POST count so it can be waited on
 *  again later
 *
*/
      rc=DosResetEventSem(master,&postct);
      if (rc)
      {
         printf("DosResetEventSem failed for Master Sem rc= %ld\n",rc);
         DosExit(EXIT_PROCESS,8);

      }
/*
 *     If its posted check the completion code from worker
 *
*/
      if (SharedMem[i].result == ITSOK)
      {
         printf("Process %ld started for database %s\n",
                   result.codeTerminate,dbNames[i]);
         Dbstate[i]=CONNECTED;
         Connectcount++;
      }
      else
         printf("Process %ld started for database %s .. but it had problems\n",
                   result.codeTerminate,dbNames[i]);
    }
    /* begin servicing user requests */
    while (Connectcount)
    {
       ULONG currentDb;
       int action;
       int value;
       int newvalue;
       int items;
       struct _updatestuff  * stuff;
       BOOL GoodRequest;
       printf("\n__________________________________________________________\n");
       printf("__________________________________________________________\n");
       printf("enter request in the form of 'database number,Action,Value'\n");
       printf("Where actions is 0 - terminate, 1 - update, 2 - insert, 3 -beep\n");
       printf("Where database number is:\n");
       for (i=0;i<numdbs;i++)
       {
          if (Dbstate[i]== CONNECTED)
             printf("%ld for %s\n",i,dbNames[i]);
       }
       printf("__________________________________________________________\n");
       printf("__________________________________________________________\n");
/*
 *    Not a very sophisticated user interface but it illustrates a point
 *    get action, communicate information to the correct worker via
 *    shared memory and semaphore, wait for completion and display results
 *
*/
/*
 *
 *
 *
*/

       items = scanf("%lu,%ld,%ld",&currentDb,&action,&value);
       if (items != EOF)
       {
          if (currentDb>numdbs || Dbstate[currentDb]== NOT_CONNECTED)
          {
             printf("The database number %ld is invalid or DB not connected\n",currentDb);
          }
          else
          {
            GoodRequest=TRUE;
/*
 *     REQTERM will tell worker to terminate
 *     REQBEEP will tell worker to beep, different frequency for different dbs
 *     REQUPDATE will take the value and use it as a key, and prompt for
 *               a value to update.
 *     REQINSERT will take the value and use it as a key, and prompt for
 *               a additonal value to insert.
 *
*/
            switch (action) {
            case REQTERM:
               Dbstate[currentDb]= NOT_CONNECTED  ;
               --Connectcount;
               break;
            case REQBEEP:
               break;
            case REQUPDATE:
              printf("Enter the new value for key %ld\n",value);
              items = scanf("%ld",&newvalue);
              if (items == EOF)
              {
               printf("Ok update request cancelled\n");
               GoodRequest=FALSE;
              }
              else
              {
                  stuff = (struct _updatestuff  *)SharedMem[currentDb].inarea;
                  stuff->key=value;
                  stuff->value=newvalue;
              }
               break;
            case REQINSERT:
              printf("Enter the new value for key %ld\n",value);
              items = scanf("%ld",&newvalue);
              if (items == EOF)
              {
               printf("Ok insert request cancelled\n");
               GoodRequest=FALSE;
              }
              else
              {
                  stuff = (struct _updatestuff  *)SharedMem[currentDb].inarea;
                  stuff->key=value;
                  stuff->value=newvalue;
              }
               break;
            default:
               printf("unsupported action %ld\n",action);
               GoodRequest=FALSE;
               break;
            } /* endswitch */
            if (GoodRequest)
            {
                SharedMem[currentDb].requestcode=action;
/*
 *   Tell the worker to look at his shared memory piece to find
 *   his task.
 *
*/
                rc=DosPostEventSem(dbSemaphores[currentDb]);
                if (rc)
                {
                   printf("DosPostEventSem failed for worker Sem rc= %ld\n",rc);
                   DosExit(EXIT_PROCESS,8);
                }
/*
 *         As I said earlier this could be made more sophisticated
 *         to handle multiple requests asynchronously
 *
*/
                rc=DosWaitEventSem(master,SEM_INDEFINITE_WAIT);
                if (rc)
                {
                   printf("DosWaitEventSem failed for Master Sem rc= %ld\n",rc);
                   DosExit(EXIT_PROCESS,8);

                }
/*
 *          Reset the post count so that I can wait on it again
 *
*/
                rc=DosResetEventSem(master,&postct);
                if (rc)
                {
                   printf("DosResetEventSem failed for Master Sem rc= %ld\n",rc);
                   DosExit(EXIT_PROCESS,8);
                }
                printf("return code and data from %s for action %ld is:\n",
                            dbNames[currentDb],action);
                printf("result->%ld data->%s\n",
                            SharedMem[currentDb].result,
                            SharedMem[currentDb].outarea);
            }
          }
       }
    }
/*
 *       Clean up and go away
 *
*/
    printf("All connections gone I am going away\n");
    DosCloseEventSem(master);
    for (i=0;i<numdbs;i++)
       DosCloseEventSem(dbSemaphores[i]);
    DosFreeMem(SharedMem);
}

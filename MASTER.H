#define SHARED_MEM_NAME "\\SHAREMEM\\MASTER.DAT"
#define MASTER_SEM_NAME "\\SEM32\\MASTER.SEM"
#define CLIENT_NAME  "worker.exe"
#define DBNAMECOUNT 3
#define REQTERM 0
#define REQUPDATE 1
#define REQINSERT 2
#define REQBEEP   3
#define NOT_CONNECTED 0
#define CONNECTED 1
#define ITSOK 0
#define ITSAWARNING 100
#define ITSBAD      -1
static char * dbNames[DBNAMECOUNT] = {"SAMPLE","DALE","TURBO"};
static HEV dbSemaphores[DBNAMECOUNT];
static int Dbstate[DBNAMECOUNT];
struct _commarea
   {
      int requestcode;
      int  result;
      char inarea[2044];
      char outarea[2044];
   };
struct _updatestuff
   {
      long key;
      long value;
   };

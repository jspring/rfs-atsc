/* edilib.c - only print_log for now.
*/

#include <sys_os.h>

int print_log(unsigned char *);

int print_log(unsigned char *ptr1) {
   char *text;

   text = (char *)ptr1;
   printf("CASE_TEXT_TO_MESSAGE: Format char %#x Cabinet status %#x\n", 
       text[0] & 0xFF,
       text[49] & 0xFF);
    printf("GREEN\n");
    printf("%#x %#x %#x %#x %#x %#x %#x %#x ", 
       text[1] & 0xFF,
       text[2] & 0xFF,
       text[3] & 0xFF,
       text[4] & 0xFF,
       text[5] & 0xFF,
       text[6] & 0xFF,
       text[7] & 0xFF,
       text[8] & 0xFF
 );
    printf("%#x %#x %#x %#x %#x %#x %#x %#x\n", 
       text[9] & 0xFF,
       text[10] & 0xFF,
       text[11] & 0xFF,
       text[12] & 0xFF,
       text[13] & 0xFF,
       text[14] & 0xFF,
       text[15] & 0xFF,
       text[16] & 0xFF
 );
    printf("YELLOW\n");
    printf("%#x %#x %#x %#x %#x %#x %#x %#x ", 
       text[17] & 0xFF,
       text[18] & 0xFF,
       text[19] & 0xFF,
       text[20] & 0xFF,
       text[21] & 0xFF,
       text[22] & 0xFF,
       text[23] & 0xFF,
       text[24] & 0xFF
 );
    printf("%#x %#x %#x %#x %#x %#x %#x %#x\n", 
       text[25] & 0xFF,
       text[26] & 0xFF,
       text[27] & 0xFF,
       text[28] & 0xFF,
       text[29] & 0xFF,
       text[30] & 0xFF,
       text[31] & 0xFF,
       text[32] & 0xFF
 );
    printf("RED\n");
    printf("%#x %#x %#x %#x %#x %#x %#x %#x ", 
       text[33] & 0xFF,
       text[34] & 0xFF,
       text[35] & 0xFF,
       text[36] & 0xFF,
       text[37] & 0xFF,
       text[38] & 0xFF,
       text[39] & 0xFF,
       text[40] & 0xFF
 );
   printf("%#x %#x %#x %#x %#x %#x %#x %#x\n", 
       text[41] & 0xFF,
       text[42] & 0xFF,
       text[43] & 0xFF,
       text[44] & 0xFF,
       text[45] & 0xFF,
       text[46] & 0xFF,
       text[47] & 0xFF,
       text[48] & 0xFF
 );
    return 0;
}//end of print_log

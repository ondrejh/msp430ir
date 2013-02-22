/**
 * this is example programm, to show howto work with ircodes.h file
 *
 * it should print out content of the file (code names and codes itself)
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "ircodes.h"

int main()
{
    // support variables (counters)
    int i,j;

    // copy of the data pointer
    const uint16_t *codesptr = ircode_data;

    for (i=0;i<IRCODES;i++)
    {
        // readout from codes data using pointer incrementation (sequential read)
        // first item is the code length
        uint16_t len = *codesptr++;

        // print code name
        printf("    Code %d: %s\n\n",i,ircode_name[i]);

        // print data length
        printf("Length: %d\nData:\n",len);

        // print data itself (with pointer incrementation inside)
        for (j=0;j<len;j++)
            printf("0x%04X%s",*codesptr++,((j==len-1)||(((j+1)%8)==0))?"\n":",");
        if (i<IRCODES-1) printf("\n\n");

        // you have to do pointer incrementation "len" times
        // to get pointer on the next "len" position
        // clear???
    }

    return 0;
}

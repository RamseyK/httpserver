#include <iostream>
#include <signal.h>

#include "Testers.h"

void handleSigPipe(int snum);

// Ignore signals with this function
void handleSigPipe(int snum) {
    return;
}

int main (int argc, const char * argv[])
{
    // Ignore SIGPIPE "Broken pipe" signals when socket connections are broken.
    signal(SIGPIPE, handleSigpipe);
    
    testAll();
    
    return 0;
}



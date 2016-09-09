**sipcallnotify uses the pjsua2 library to register with a sip provider, make a call, play a recorded message, then hang up**

**Usage:**

   ./sipcallnotify  [-t <int>] [-r <int>] [-m <string>] [-p <string>] [-u
                    <string>] [-o <int>] [-c <string>] [-s <string>] [--]
                    [--version] [-h]


Where: 

   -t <int>,  --timeout <int>
     max call time in seconds

   -r <int>,  --repeat <int>
     number of times to play media file

   -m <string>,  --mediafile <string>
     path of .wav file

   -p <string>,  --password <string>
     password from sip provider

   -u <string>,  --user <string>
     username from sip provider

   -o <int>,  --port <int>
     sip port

   -c <string>,  --calluri <string>
     call uri

   -s <string>,  --sipprovider <string>
     sip provider server

   --,  --ignore_rest
     Ignores the rest of the labeled arguments following this flag.

   --version
     Displays version information and exits.

   -h,  --help
     Displays usage information and exits.


   Make a sip call and play a notification message

__________________________________________________________________

**Building**  
  
 Dependencies  
   
    pjsip: http://www.pjsip.org/download.htm  
    Extract source tar to a sibling directory of sipcallnotify source  
    Build pjsip with ./configure --disable-video && make dep && make  
     
    tclap: https://sourceforge.net/projects/tclap/files/  
    Extract source tar to a sibling directory of sipcallnotify source  
    No building necessary  

    libasound2-dev should be installed from your distribution repo  

  Make  

    cd sipcallnotify && make

__________________________________________________________________
  
**Notes**
  
  The mediafile must be a 16 bit mono PCM  

  
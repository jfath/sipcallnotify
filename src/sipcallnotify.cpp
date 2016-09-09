/*
sipcallnotify.cpp
Utility to make a sip call, play a recorded message, then disconnect
Uses the pjsua2 library from http://www.pjsip.org/
Based on pjsua2_demo.cpp which is Copyright (C) 2008-2013 Teluu Inc. (http://www.teluu.com)

Copyright (c) 2016 Jerry Fath

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include <pjsua2.hpp>
#include <iostream>
#include <memory>
#include <pj/file_access.h>
#include <tclap/CmdLine.h>
#include <sstream>

#define SSTR( x ) static_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

#define THIS_FILE "sipcall.cpp"

using namespace pj;

class MyAccount;
class MyAudioMediaPlayer;
class RoboCall;

class GlobalData
{
    public:
        GlobalData()
        {
            callState = 0;
        }

        ~GlobalData()
        {
        }

        //Call state machine tracker
        int callState;

        //Command line args
        std::string provider;
        std::string calluri;
        int port;
        std::string username;
        std::string password;
        std::string mediafile;
        int repeat;
        int timeout;
};

class MyAudioMediaPlayer : public AudioMediaPlayer
{
  private:
    GlobalData *myGlobals;
    int repeatCount;

    public:
        MyAudioMediaPlayer()
        {
            repeatCount=0;
        }

        void createPlay(AudioMedia &aud_med, GlobalData &globals_)
        {
            myGlobals = (GlobalData *)&globals_;
            createPlayer(myGlobals->mediafile);
            startTransmit(aud_med);
        }

        bool onEof()
        {
            repeatCount++;
            if (repeatCount >= myGlobals->repeat)
            {
                myGlobals->callState = 3;
                return false;
            } 
            else return true;
        }

};


class RoboCall : public Call
{
  private:
    MyAccount *myAcc;
    GlobalData *myGlobals;
    MyAudioMediaPlayer myAmp;

  public:
    RoboCall(Account &acc, GlobalData &globals_, int call_id = PJSUA_INVALID_ID)
        : Call(acc, call_id)
    {
        myAcc = (MyAccount *)&acc;
        myGlobals = (GlobalData *)&globals_;
    }

    virtual void onCallState(OnCallStateParam &prm);
};

class MyAccount : public Account
{
  private:
    GlobalData *myGlobals;

  public:
    std::vector<Call *> calls;

  public:
    MyAccount(GlobalData &globals_)
    {
        myGlobals = (GlobalData *)&globals_;
    }

    ~MyAccount()
    {
        std::cout << "*** Account is being deleted: No of calls="
                  << calls.size() << std::endl;
    }

    void removeCall(Call *call)
    {
        for (std::vector<Call *>::iterator it = calls.begin();
             it != calls.end(); ++it)
        {
            if (*it == call)
            {
                calls.erase(it);
                break;
            }
        }
    }

    virtual void onRegState(OnRegStateParam &prm)
    {
        AccountInfo ai = getInfo();
        if (ai.regIsActive && (myGlobals->callState < 1)) myGlobals->callState = 1;
        std::cout << (ai.regIsActive ? "*** Register: code=" : "*** Unregister: code=")
                  << prm.code << std::endl;
    }

    virtual void onIncomingCall(OnIncomingCallParam &iprm)
    {
        std::cout << "*** Incoming Call: " << std::endl;
    }
};

void RoboCall::onCallState(OnCallStateParam &prm)
{
    PJ_UNUSED_ARG(prm);

    CallInfo ci = getInfo();
    std::cout << "*** Call: " << ci.remoteUri << " [" << ci.stateText
              << "]" << std::endl;

    if (ci.state == PJSIP_INV_STATE_CONFIRMED)
    {
        CallInfo ci = getInfo();
        // Iterate all the call medias
        for (unsigned i = 0; i < ci.media.size(); i++)
        {
            if (ci.media[i].type == PJMEDIA_TYPE_AUDIO && getMedia(i))
            {
                if (myGlobals->callState < 2)
                {
                    myGlobals->callState = 2;
                    AudioMedia *aud_med = (AudioMedia *)getMedia(i);
                    myAmp.createPlay(*aud_med, *myGlobals);
                }
            }
        }
    }

    if (ci.state == PJSIP_INV_STATE_DISCONNECTED)
    {
        myAcc->removeCall(this);
        /* Delete the call */
        delete this;
    }
}

//
// Register with sip provider, make a call, play a message, hang up
//
int DoRoboCall(GlobalData &globals_) throw(Error)
{
    Endpoint ep;
    int ret=0;

    try
    {
        ep.libCreate();

        // Init library
        EpConfig ep_cfg;
        //!!!Things we can do to change logging
        //ep_cfg.msgLogging = 1;
        //ep_cfg.logConfig.level = 5;
        //ep_cfg.consoleLevel = 4;
        //ep_cfg.filename = "sipcallnotify.log"
        ep.libInit(ep_cfg);

        // Create transport
        TransportConfig tcfg;
        tcfg.port = globals_.port;
        ep.transportCreate(PJSIP_TRANSPORT_UDP, tcfg);
        ep.transportCreate(PJSIP_TRANSPORT_TCP, tcfg);

        // Start library
        ep.libStart();
        std::cout << "*** PJSUA2 STARTED ***" << std::endl;

        //!!!Can we use null device and get rid of alsa dependency?
        //setCaptureDev(int capture_dev)
        //setPlaybackDev(int playback_dev)

        // Add account
        AccountConfig acc_cfg;
        //acc_cfg.idUri = "sip:4535413348@sip.anveo.com:5010";
        acc_cfg.idUri = "sip:" + globals_.username + "@" + globals_.provider + ":" + SSTR(globals_.port);
        //acc_cfg.regConfig.registrarUri = "sip:sip.anveo.com:5010";
        acc_cfg.regConfig.registrarUri = "sip:" + globals_.provider + ":" + SSTR(globals_.port);

        AuthCredInfo aci;
        aci.scheme = "digest";
        aci.username = globals_.username;
        aci.data = globals_.password;
        aci.dataType = 0;
        aci.realm = "*";
        acc_cfg.sipConfig.authCreds.push_back(aci);

        std::auto_ptr<MyAccount> acc(new MyAccount(globals_));
        acc->create(acc_cfg);

        //Sleep and wait for a account registered
        for (int i=0; i<5; ++i) {
            pj_thread_sleep(1000);
            if (globals_.callState > 0) break;
        }
        if (globals_.callState <= 0) PJSUA2_RAISE_ERROR3(PJ_ETIMEDOUT, "DoRoboCall()", "Registration timed out");

        // Make outgoing call
        //std::auto_ptr<RoboCall> call(new RoboCall(*acc, globals_));
        Call *call = new RoboCall(*acc, globals_);
        acc->calls.push_back(call);
        CallOpParam prm(true);
        prm.opt.audioCount = 1;
        prm.opt.videoCount = 0;
        call->makeCall(globals_.calluri, prm);

        //Sleep and wait loop for media player to finish
        for (int i=0; i < globals_.timeout; ++i) {
            pj_thread_sleep(1000);
            if (globals_.callState > 2) break;
        }
        //Call timeout isn't a hard error
        //if (globals_.callState <= 2) PJSUA2_RAISE_ERROR3(PJ_ETIMEDOUT, "DoRoboCall()", "Call timed out");

        // Hangup all calls
        ep.hangupAllCalls();
        pj_thread_sleep(10000);

        //std::string tmpstr;
        //std::cout << "Press enter to continue ";
        //std::getline(std::cin, tmpstr);

        ret = PJ_SUCCESS;
    }
    catch (Error &err)
    {
        std::cout << "Exception: " << err.info() << std::endl;
        ret = 1;
    }

    try
    {
        ep.libDestroy();
    }
    catch (Error &err)
    {
        std::cout << "Exception: " << err.info() << std::endl;
        ret = 1;
    }

    return ret;
}

//
// Parse command line, then call DoRoboCall for all of the hard work
//
int main(int argc, char** argv)
{
    int ret = 0;

    //Storage for variables needed by multiple classes
    std::auto_ptr<GlobalData> globals_(new GlobalData);

    //Parse command line using tclap
    try
    {

        // Define the command line object, and insert a message
        // that describes the program.
        TCLAP::CmdLine cmd("Make a sip call and play a notification message", ' ', "1.0");

        // Define arguments and add to the CmdLine object
        TCLAP::ValueArg<std::string> providerArg("s", "sipprovider", "sip provider domain name", false, "sip.anveo.com", "string", cmd);
        TCLAP::ValueArg<std::string> calluriArg("c", "calluri", "call uri (sip:#-###-###-####@sip.anveo.com:5010)", true, "sip:#-###-###-####@sip.anveo.com:5010", "string", cmd);
        TCLAP::ValueArg<int> portArg("o", "port", "sip port", false, 5010, "int", cmd);
        TCLAP::ValueArg<std::string> usernameArg("u", "user", "username from sip provider", true, "YOURUSERNAME", "string", cmd);
        TCLAP::ValueArg<std::string> passwordArg("p", "password", "password from sip provider", true, "YOURPASSWORD", "string", cmd);
        TCLAP::ValueArg<std::string> mediafileArg("m", "mediafile", "path of .wav file", false, "sipcallnotify.wav", "string", cmd);
        TCLAP::ValueArg<int> repeatArg("r", "repeat", "number of times to play media file", false, 1, "int", cmd);
        TCLAP::ValueArg<int> timeoutArg("t", "timeout", "max call time in seconds", false, 20, "int", cmd);

        // Parse the argv array.
        cmd.parse(argc, argv);

        // Get the value parsed by each arg.
        globals_->provider = providerArg.getValue();
        globals_->calluri = calluriArg.getValue();
        globals_->port = portArg.getValue();
        globals_->username = usernameArg.getValue();
        globals_->password = passwordArg.getValue();
        globals_->mediafile = mediafileArg.getValue();
        globals_->repeat = repeatArg.getValue();
        globals_->timeout = timeoutArg.getValue();

    }
    catch (TCLAP::ArgException &e) // catch any exceptions
    {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
        return 1;
    }

    ret = DoRoboCall(*globals_);

    if (ret == PJ_SUCCESS)
    {
        std::cout << "Success" << std::endl;
    }
    else
    {
        std::cout << "Error Found" << std::endl;
    }

    return ret;
}

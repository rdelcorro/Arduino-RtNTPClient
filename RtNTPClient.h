#pragma once

#include "Arduino.h"

#include <Udp.h>

#define SEVENZYYEARS 2208988800UL
#define NTP_PACKET_SIZE 48
#define NTP_DEFAULT_LOCAL_PORT 1337
#define NTP_DEFAULT_SERVER "time.nist.gov"
#define NTP_DEFAULT_TIME_OFFSET 0
#define NTP_DEFAULT_UPDATE_INTERVAL 300000
#define NTP_DEFAULT_TIMEOUT 1000

typedef void (*PrintFunc) (const char * format, ...);

class RtNTPClient {
  public:
    RtNTPClient(UDP& udp, const char* poolServerName=NTP_DEFAULT_SERVER, 
              int timeOffset=0, int updateInterval=NTP_DEFAULT_UPDATE_INTERVAL,
              int port=NTP_DEFAULT_LOCAL_PORT);

    ~RtNTPClient();

    /**
     * This should be called in the main loop of your application. By default an update from the NTP Server is only
     * made every NTP_DEFAULT_UPDATE_INTERVAL seconds. This can be configured in the RtNTPClient constructor.
     *
     * @return true on success, false on failure
     */
    bool update();

    /**
     * This will force the update from the NTP Server.
     *
     * @return true on success, false on failure
     */
    bool forceUpdate();

    int getDay();
    int getHours();
    int getMinutes();
    int getSeconds();

    /**
     * Changes the time offset. Useful for changing timezones dynamically
     */
    void setTimeOffset(int timeOffset);

    /**
     * Set the update interval to another frequency. E.g. useful when the
     * timeOffset should not be set in the constructor
     */
    void setUpdateInterval(int updateInterval);

    /**
     * @return time formatted like `hh:mm:ss`
     */
    String getFormattedTime();

    /**
     * @return time in seconds since Jan. 1, 1970
     */
    unsigned long getEpochTime();

    /**
     * Set a debug function to print messages such as Serial.printf
     * The func needs to follow the printf signature. Check the sample for more information
     */    
    void setPrintDebugFunc(PrintFunc func);

  private:
    UDP*          mUdp;

    const char*   mPoolServerName; 
    int           mPort;
    int           mTimeOffset;

    unsigned int  mUpdateInterval;  // In ms
    unsigned long mCurrentEpoc = 0; // In s
    unsigned long mLastSync = 0;    // In ms

    byte mPacketBuffer[NTP_PACKET_SIZE];

    bool mPacketSent = false;
    byte mReceivedLen = 0;
    bool mUpdateTime = true;
    unsigned long mLastPacketSent = 0;

    PrintFunc mPrintFunc = nullptr;

    void sendNTPPacket();
    void printMsg(const char * format, ...);
};

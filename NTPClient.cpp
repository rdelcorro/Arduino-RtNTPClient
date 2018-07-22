/**
 * The MIT License (MIT)
 * Copyright (c) 2015 by Fabrice Weinberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "RtNTPClient.h"

RtNTPClient::RtNTPClient(UDP& udp, const char* poolServerName, int timeOffset, int updateInterval, int port) 
  : mUdp(&udp) {
  mTimeOffset     = timeOffset;
  mPoolServerName = poolServerName;
  mUpdateInterval = updateInterval;
  mUdp->begin(port);
}

RtNTPClient::~RtNTPClient() {
  mUdp->stop();
}

bool RtNTPClient::forceUpdate() {
  // This function should not block or it will cause unintended timing problems in programs
  // The function will try its best to get the response but never block

  // UDP packets can be lost, so keep trying if a timeout occurs
  if (millis() - mLastPacketSent > NTP_DEFAULT_TIMEOUT) {
    mPacketSent = false;
  } 

  if (mPacketSent == false) {
    sendNTPPacket();
    mPacketSent = true;
    mLastPacketSent = millis();
  }

  const int pktSize = mUdp->parsePacket();
  if (pktSize == 0) { // No data was received
    return false;
  }
  printMsg("Parse packet done\n");

  const int len = mUdp->read(mPacketBuffer, NTP_PACKET_SIZE);
  mReceivedLen += len;
  if (mReceivedLen == NTP_PACKET_SIZE) {
    // We are done here
    mLastSync = millis();
    const unsigned long highWord = word(mPacketBuffer[40], mPacketBuffer[41]);
    const unsigned long lowWord = word(mPacketBuffer[42], mPacketBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    const unsigned long secsSince1900 = highWord << 16 | lowWord;
    mCurrentEpoc = secsSince1900 - SEVENZYYEARS;

    // Reset the flags
    mPacketSent = false;
    mReceivedLen = 0;
    printMsg("NTP sync complete\n");

    return true;
  }

  return false;
}

bool RtNTPClient::update() {
  if (millis() - mLastSync >= mUpdateInterval || mPacketSent == false) {                                                     
    return this->forceUpdate();
  }
  return false;
}

// NOTE this does not deal with millis rollover
unsigned long RtNTPClient::getEpochTime() {
  return mTimeOffset + // User offset
         mCurrentEpoc + // Epoc returned by the NTP server
         ((millis() - mLastSync) / 1000); // Time since last update
}

int RtNTPClient::getDay() {
  return (((getEpochTime()  / 86400L) + 4 ) % 7); //0 is Sunday
}
int RtNTPClient::getHours() {
  return ((getEpochTime()  % 86400L) / 3600);
}
int RtNTPClient::getMinutes() {
  return ((getEpochTime() % 3600) / 60);
}
int RtNTPClient::getSeconds() {
  return (getEpochTime() % 60);
}

String RtNTPClient::getFormattedTime() {
  unsigned long rawTime = this->getEpochTime();
  unsigned long hours = (rawTime % 86400L) / 3600;
  String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

  unsigned long minutes = (rawTime % 3600) / 60;
  String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

  unsigned long seconds = rawTime % 60;
  String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

  return hoursStr + ":" + minuteStr + ":" + secondStr;
}

void RtNTPClient::setTimeOffset(int timeOffset) {
  mTimeOffset = timeOffset;
}

void RtNTPClient::setUpdateInterval(int updateInterval) {
  mUpdateInterval = updateInterval;
}

void RtNTPClient::sendNTPPacket() {
  printMsg("Sending the ntp packet");
  // set all bytes in the buffer to 0
  memset(mPacketBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  mPacketBuffer[0] = 0b11100011;   // LI, Version, Mode
  mPacketBuffer[1] = 0;     // Stratum, or type of clock
  mPacketBuffer[2] = 6;     // Polling Interval
  mPacketBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  mPacketBuffer[12]  = 49;
  mPacketBuffer[13]  = 0x4E;
  mPacketBuffer[14]  = 49;
  mPacketBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  mUdp->beginPacket(mPoolServerName, 123); //NTP requests are to port 123
  mUdp->write(mPacketBuffer, NTP_PACKET_SIZE);
  mUdp->endPacket();
}

void RtNTPClient::setPrintDebugFunc(PrintFunc func) {
  mPrintFunc = func;
}

void RtNTPClient::printMsg(const char *fmt, ...) {
  if (mPrintFunc != nullptr) {
    va_list args;
    va_start(args, fmt);
    (*mPrintFunc)(fmt, args);
    va_end(args);
  }
}
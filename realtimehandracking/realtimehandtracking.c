/* Copyright (C) 2012-2017 Ultraleap Limited. All rights reserved.
 *
 * Use of this code is subject to the terms of the Ultraleap SDK agreement
 * available at https://central.leapmotion.com/agreements/SdkAgreement unless
 * Ultraleap has signed a separate license agreement with you or your
 * organisation.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <Windows.h>
#include <stdint.h>
int gettimeofday(struct timeval* tp, struct timezone* tzp)
{
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
    // until 00:00:00 January 1, 1970
    static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);
    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;
    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    time = ((uint64_t)file_time.dwLowDateTime);
    time += ((uint64_t)file_time.dwHighDateTime) << 32;
    tp->tv_sec = (long)((time - EPOCH) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
    return 0;
}
#else
#include <unistd.h>
#endif
#include "LeapC.h"
#include "ExampleConnection.h"
int64_t lastFrameID = 0; //The last frame received
 /** Callback for when the connection opens. */
static void OnConnect() {
    printf("Connected.\n");
}
/** Callback for when a device is found. */
static void OnDevice(const LEAP_DEVICE_INFO* props) {
    printf("Found device %s.\n", props->serial);
}
/** Callback for when a frame of tracking data is available. */
static void OnFrame(const LEAP_TRACKING_EVENT* frame) {
    if (frame->info.frame_id % 60 == 0)
        printf("Frame %lli with %i hands.\n", (long long int)frame->info.frame_id, frame->nHands);
    for (uint32_t h = 0; h < frame->nHands; h++) {
        LEAP_HAND* hand = &frame->pHands[h];
        struct timeval ts;
        gettimeofday(&ts, NULL); // return value can be ignored
        printf("Timestamp : %ld sec %ld usec\n", ts.tv_sec, ts.tv_usec);
        printf("Hand id %i: %s\n",
            hand->id,
            (hand->type == eLeapHandType_Left ? "left" : "right"));
        printf("Palm: %f, %f, %f\n",
            hand->palm.position.x,
            hand->palm.position.y,
            hand->palm.position.z);
        printf("Arm: %f, %f, %f, %f, %f, %f\n",
            hand->arm.prev_joint.x,
            hand->arm.prev_joint.y,
            hand->arm.prev_joint.z,
            hand->arm.next_joint.x,
            hand->arm.next_joint.y,
            hand->arm.next_joint.z);
    }
}
/** Callback for when an image is available. */
static void OnImage(const LEAP_IMAGE_EVENT* imageEvent) {
    printf("Received image set for frame %lli with size %lli.\n",
        (long long int)imageEvent->info.frame_id,
        (long long int)imageEvent->image[0].properties.width *
        (long long int)imageEvent->image[0].properties.height * 2);
}
int main(int argc, char** argv) {
    //Set callback function pointers
    ConnectionCallbacks.on_connection = &OnConnect;
    ConnectionCallbacks.on_device_found = &OnDevice;
    ConnectionCallbacks.on_frame = &OnFrame;
    //ConnectionCallbacks.on_image = &OnImage;
    LEAP_CONNECTION* connection = OpenConnection();
    LeapSetPolicyFlags(*connection, eLeapPolicyFlag_Images, 0);
    printf("Press Enter to exit program.\n");
    getchar();
    CloseConnection();
    DestroyConnection();
    return 0;
}
//End-of-Sample
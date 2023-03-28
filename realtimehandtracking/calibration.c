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
#include <string.h>
#include <math.h>
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
#include <sys/time.h>
#endif

#include "LeapC.h"
#include "ExampleConnection.h"
#include "handtracking.h"

FILE *calibrateFile;
int isConnected = 0;
int isDeviceFound = 0;

 /** Callback for when the connection opens. */
static void OnConnect() {
    printf("Connected.\n");
    isConnected = 1;
}
/** Callback for when a device is found. */
static void OnDevice(const LEAP_DEVICE_INFO* props) {
    printf("Found device %s.\n", props->serial);
    isDeviceFound = 1;
}

Plane calculatePlane(Vector p1, Vector p2, Vector p3) {

    Vector u;
    u.x = p1.x - p2.x;
    u.y = p1.y - p2.y;
    u.z = p1.z - p2.z;

    Vector v;
    v.x = p1.x - p3.x;
    v.y = p1.y - p3.y;
    v.z = p1.z - p3.z;

    Vector n;
    n.x = u.y * v.z - u.z * v.y;
    n.y = u.z * v.x - u.x * v.z;
    n.z = u.x * v.y - u.y * v.x;

    Plane plane;
    plane.a = n.x;
    plane.b = n.y;
    plane.c = n.z;
    plane.d = n.x * -p1.x + n.y * -p1.y + n.z * -p1.z;

    return plane;
}

float distanceFromPlane(Plane plane, Vector p) {
    return fabs(plane.a * p.x + plane.b * p.y + plane.c * p.z + plane.d) / sqrt(plane.a * plane.a + plane.b * plane.b + plane.c * plane.c);
}


Vector getVector(char* prompt) {

    Vector ret;

    while (1) {
        printf("%s\n", prompt);
        for (uint32_t i = 10; i >= 1; i--) {
            printf("%d.\n", i);
            sleep(1);
        }
        LEAP_TRACKING_EVENT* frame = GetFrame();
        if (frame->nHands > 0) {

            LEAP_VECTOR position = frame->pHands[0].palm.position;
            ret.x = position.x;
            ret.y = position.y;
            ret.z = position.z;
            break;
        }
    }

    printf("Captured Coordinate.\n");
    return ret;
}
int main(int argc, char** argv) {

    calibrateFile = fopen("calibrate.txt", "w");

    //Set callback function pointers
    ConnectionCallbacks.on_connection = &OnConnect;
    ConnectionCallbacks.on_device_found = &OnDevice;
    LEAP_CONNECTION* connection = OpenConnection();
    LeapSetPolicyFlags(*connection, eLeapPolicyFlag_Images, 0);
    
    while (!isConnected || !isDeviceFound) { sleep(0.01); }

    Vector p1, p2, p3;
    Plane plane;
    p1 = getVector("Hold your palm against the top left corner of the screen.");
    p2 = getVector("Hold your palm against the bottom middle side of the screen.");
    p3 = getVector("Hold your palm against the middle right side of the screen.");
    plane = calculatePlane(p1, p2, p3);

    fprintf(calibrateFile, "%f %f %f %f", plane.a, plane.b, plane.c, plane.d);

    // Cleanup
    fclose(calibrateFile);
    CloseConnection();
    DestroyConnection();
    return 0;
}

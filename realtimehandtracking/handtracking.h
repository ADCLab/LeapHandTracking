
#ifdef _WIN32
#include <stdint.h>
#else
#include <unistd.h>
#endif

typedef struct {
    float a;
    float b;
    float c;
    float d;
} Plane; // ax + by + cz + d = 0

typedef struct {
    float x;
    float y;
    float z;
} Vector;

typedef struct {
  char *response;
  size_t size;

} memory;

typedef struct {
    char userId[50];
    uint32_t sequence;
    char studyStage[50];
    char destinationIndex[50];
    char departureIndex[50];
    uint32_t decisionState;
    uint32_t vitalsState;
    uint32_t airspaceState;
    uint32_t flightStartTime;
    uint32_t resetUserDisplay;
    uint32_t resetVitalsDisplay;
    uint32_t timeToDestination;
    uint32_t preTrial;
    double latitude;
    double longitude;
    uint32_t compass;

} flaskData;

Plane calculatePlane(Vector p1, Vector p2, Vector p3);
float distanceFromPlane(Plane plane, Vector p);

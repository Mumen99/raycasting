#ifndef SYNCHRONIZATION_H
#define SYNCHRONIZATION_H

#include "network.h"

typedef enum
{
    SYNC_NO_SMOOTHING = 0,
    SYNC_INTERPOLATION,
    SYNC_EXTRAPOLATION,
    SYNC_EXPONENTIAL_SMOOTHING

} SynchronizationMode;

void Synchronization_SetMode(
    SynchronizationMode mode);

SynchronizationMode Synchronization_GetMode(void);

int Synchronization_ProcessPlayers(
    const RemotePlayerState *network_states,
    int network_count,
    RemotePlayerState *output_states,
    int max_output_states);

#endif

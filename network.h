#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>

#define MAX_REMOTE_PLAYERS 4

typedef struct
{
    int player_id;

    float x;
    float y;
    float angle;

    /*
     * Increases every time a new network update
     * is received for this player.
     *
     * This lets synchronization.c detect a new
     * packet even when the player's position
     * has not changed.
     */
    unsigned int update_id;

    bool active;

} RemotePlayerState;

void Network_Init(const char *server_url);

void Network_SendPlayerState(
    float x,
    float y,
    float angle);

bool Network_IsConnected(void);

int Network_GetLocalPlayerId(void);

int Network_GetRemotePlayers(
    RemotePlayerState *out_states,
    int max_states);

void Network_Shutdown(void);

#endif

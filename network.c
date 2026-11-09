#include "network.h"

#include <stdio.h>
#include <string.h>

#include <emscripten.h>
#include <emscripten/websocket.h>

static EMSCRIPTEN_WEBSOCKET_T socket_handle = 0;
static bool connected = false;
static int local_player_id = -1;

static RemotePlayerState remote_players[MAX_REMOTE_PLAYERS];

static void ClearRemotePlayers(void)
{
    for (int i = 0; i < MAX_REMOTE_PLAYERS; i++)
    {
        remote_players[i].player_id = -1;
        remote_players[i].x = 0.0f;
        remote_players[i].y = 0.0f;
        remote_players[i].angle = 0.0f;
        remote_players[i].active = false;
        remote_players[i].update_id = 0;
    }
}

static int FindPlayerSlot(int player_id)
{
    for (int i = 0; i < MAX_REMOTE_PLAYERS; i++)
    {
        if (remote_players[i].active &&
            remote_players[i].player_id == player_id)
        {
            return i;
        }
    }
    return -1;
}

static int FindFreeSlot(void)
{
    for (int i = 0; i < MAX_REMOTE_PLAYERS; i++)
    {
        if (!remote_players[i].active)
        {
            return i;
        }
    }
    return -1;
}

static EM_BOOL OnOpen(
    int event_type,
    const EmscriptenWebSocketOpenEvent *event,
    void *user_data)
{
    connected = true;
    printf("[Network] Connected to server.\n");
    return EM_TRUE;
}

static EM_BOOL OnMessage(
    int event_type,
    const EmscriptenWebSocketMessageEvent *event,
    void *user_data)
{
    if (!event->isText)
    {
        return EM_TRUE;
    }

    char message[256];

    size_t copy_length = event->numBytes;

    if (copy_length >= sizeof(message))
    {
        copy_length = sizeof(message) - 1;
    }

    memcpy(
        message,
        event->data,
        copy_length);

    message[copy_length] = '\0';

    if (strncmp(message, "WELCOME|", 8) == 0)
    {
        int received_id;

        if (
            sscanf(
                message,
                "WELCOME|%d",
                &received_id) == 1)
        {
            local_player_id = received_id;

            printf(
                "[Network] Assigned Player ID: %d\n",
                local_player_id);
        }

        return EM_TRUE;
    }

    if (strncmp(message, "PLAYER|", 7) == 0)
    {
        int sender_id;

        float x;
        float y;
        float angle;

        int values_read = sscanf(
            message,
            "PLAYER|%d|%f|%f|%f",
            &sender_id,
            &x,
            &y,
            &angle);

        if (values_read == 4)
        {
            int slot = FindPlayerSlot(sender_id);

            if (slot < 0)
            {
                slot = FindFreeSlot();
            }

            if (slot >= 0)
            {
                remote_players[slot].player_id = sender_id;

                remote_players[slot].x = x;
                remote_players[slot].y = y;
                remote_players[slot].angle = angle;

                /*
                 * Mark that another network update arrived.
                 * This increases even when position/angle
                 * stays exactly the same.
                 */
                remote_players[slot].update_id++;

                remote_players[slot].active = true;
            }
        }
        else
        {
            printf(
                "[Network] Invalid PLAYER message: %s\n",
                message);
        }

        return EM_TRUE;
    }

    if (strncmp(message, "LEAVE|", 6) == 0)
    {
        int left_id;

        if (sscanf(message, "LEAVE|%d", &left_id) == 1)
        {
            int slot = FindPlayerSlot(left_id);

            if (slot >= 0)
            {
                remote_players[slot].active = false;
                remote_players[slot].player_id = -1;
                remote_players[slot].update_id = 0;
            }
        }

        return EM_TRUE;
    }

    printf(
        "[Network] Unknown message: %s\n",
        message);

    return EM_TRUE;
}

static EM_BOOL OnError(
    int event_type,
    const EmscriptenWebSocketErrorEvent *event,
    void *user_data)
{
    printf("[Network] WebSocket error.\n");
    return EM_TRUE;
}

static EM_BOOL OnClose(
    int event_type,
    const EmscriptenWebSocketCloseEvent *event,
    void *user_data)
{
    connected = false;
    local_player_id = -1;

    ClearRemotePlayers();

    printf(
        "[Network] Connection closed. Code: %d\n",
        event->code);

    if (socket_handle > 0)
    {
        emscripten_websocket_delete(
            socket_handle);

        socket_handle = 0;
    }

    return EM_TRUE;
}

void Network_Init(
    const char *server_url)
{
    connected = false;
    local_player_id = -1;

    ClearRemotePlayers();

    if (!emscripten_websocket_is_supported())
    {
        printf(
            "[Network] WebSockets are not supported.\n");

        return;
    }

    EmscriptenWebSocketCreateAttributes attributes;

    emscripten_websocket_init_create_attributes(
        &attributes);

    attributes.url = server_url;
    attributes.protocols = NULL;
    attributes.createOnMainThread = EM_TRUE;

    socket_handle =
        emscripten_websocket_new(
            &attributes);

    if (socket_handle <= 0)
    {
        printf(
            "[Network] Failed to create WebSocket.\n");

        socket_handle = 0;
        return;
    }

    emscripten_websocket_set_onopen_callback(
        socket_handle,
        NULL,
        OnOpen);

    emscripten_websocket_set_onmessage_callback(
        socket_handle,
        NULL,
        OnMessage);

    emscripten_websocket_set_onerror_callback(
        socket_handle,
        NULL,
        OnError);

    emscripten_websocket_set_onclose_callback(
        socket_handle,
        NULL,
        OnClose);

    printf(
        "[Network] Attempting connection to %s...\n",
        server_url);
}

void Network_SendPlayerState(
    float x,
    float y,
    float angle)
{
    if (
        !connected ||
        socket_handle <= 0)
    {
        return;
    }

    char message[128];

    snprintf(
        message,
        sizeof(message),
        "STATE|%.2f|%.2f|%.4f",
        x,
        y,
        angle);

    EMSCRIPTEN_RESULT result =
        emscripten_websocket_send_utf8_text(
            socket_handle,
            message);

    if (result != EMSCRIPTEN_RESULT_SUCCESS)
    {
        printf(
            "[Network] Failed to send player state.\n");
    }
}

bool Network_IsConnected(void)
{
    return connected;
}

int Network_GetLocalPlayerId(void)
{
    return local_player_id;
}

int Network_GetRemotePlayers(
    RemotePlayerState *out_states,
    int max_states)
{
    if (out_states == NULL)
    {
        return 0;
    }

    int count = 0;

    for (int i = 0; i < MAX_REMOTE_PLAYERS && count < max_states; i++)
    {
        if (remote_players[i].active)
        {
            out_states[count++] = remote_players[i];
        }
    }

    return count;
}

void Network_Shutdown(void)
{
    if (socket_handle > 0)
    {
        emscripten_websocket_close(
            socket_handle,
            1000,
            "Client shutting down");
    }
}

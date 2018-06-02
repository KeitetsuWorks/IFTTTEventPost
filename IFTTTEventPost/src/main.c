/**
 * @file        main.c
 * @brief       Post a http request to IFTTT Maker Webhooks service
 * @author      Keitetsu
 * @date        2018/01/29
 * @copyright   Copyright (c) 2018 Keitetsu
 * @par         License
 *              This software is released under the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <tchar.h>
#include <Windows.h>


#define BUF_LEN 256


typedef struct url_st {
    TCHAR host[BUF_LEN];
    TCHAR path[BUF_LEN];
    TCHAR query[BUF_LEN];
    TCHAR fragment[BUF_LEN];
    unsigned short port;
} URL_T;


const char *dayOfWeek[] = {
    "Sun",
    "Mon",
    "The",
    "Wed",
    "Thu",
    "Fri",
    "Sat"
};


int _tmain(int argc, _TCHAR* argv[])
{
    URL_T iftttUrl = {
        _T("maker.ifttt.com"),
        _T("/trigger/{event}/with/key/{secret key}"),
        _T(""),
        _T(""),
        80
    };
    ADDRINFOT iftttAddrHints, *iftttAddrRes;
    WSADATA wsaData;
    SOCKET iftttSocket;
    char sendBuf[BUF_LEN];
    char valueDataBuf[BUF_LEN];
    char recvBuf[BUF_LEN];
    int recvSize;
    __time64_t time64Val;
    struct tm localTimeInfo;
    int retVal;

    /* ロケールの設定 */
    _tsetlocale(LC_ALL, _T("Japanese_Japan.932"));

    /* 時刻情報の取得 */
    _time64(&time64Val);
    retVal = _localtime64_s(&localTimeInfo, &time64Val);
    if (retVal != 0) {
        _tprintf(_T("Error:  _localtime64_s failed\n"));
        exit(EXIT_FAILURE);
    }

    /* Winsockの初期化 */
    retVal = WSAStartup(MAKEWORD(2, 0), &wsaData);
    if (retVal != 0) {
        _tprintf(_T("Error:  WSAStartup failed\n"));
        exit(EXIT_FAILURE);
    }

    /* ホスト名の名前解決 */
    ZeroMemory(&iftttAddrHints, sizeof(iftttAddrHints));
    iftttAddrHints.ai_family = AF_INET;
    iftttAddrHints.ai_socktype = SOCK_STREAM;

    retVal = GetAddrInfo(iftttUrl.host, _T("http"), &iftttAddrHints, &iftttAddrRes);
    if (retVal != 0) {
        _tprintf(_T("Error:  hostname resolution failed: %d\n"), retVal);
        exit(EXIT_FAILURE);
    }

    /* ソケットの作成 */
    iftttSocket = socket(iftttAddrRes->ai_family, iftttAddrRes->ai_socktype, iftttAddrRes->ai_protocol);
    if (iftttSocket == INVALID_SOCKET) {
        _tprintf(_T("Error:  socket creation failed: %d\n"), WSAGetLastError());
        exit(EXIT_FAILURE);
    }

    /* 接続 */
    retVal = connect(iftttSocket, iftttAddrRes->ai_addr, iftttAddrRes->ai_addrlen);
    if (retVal == -1) {
        _tprintf(_T("Error:  connection failed\n"));
        exit(EXIT_FAILURE);
    }

    /* サーバにリクエストを送信 */
    sprintf_s(sendBuf, sizeof(sendBuf), "POST %ls%ls HTTP/1.0\r\n", iftttUrl.path, iftttUrl.query);
    send(iftttSocket, sendBuf, (int)strlen(sendBuf), 0);

    sprintf_s(sendBuf, sizeof(sendBuf), "Host: %ls:%d\r\n", iftttUrl.host, iftttUrl.port);
    send(iftttSocket, sendBuf, (int)strlen(sendBuf), 0);

    sprintf_s(valueDataBuf, sizeof(valueDataBuf), "{\"value1\":\"%04d/%02d/%02d\",\"value2\":\"%hs\",\"value3\":\"%02d:%02d:%02d\"}\r\n",
        localTimeInfo.tm_year + 1900,
        localTimeInfo.tm_mon + 1,
        localTimeInfo.tm_mday,
        dayOfWeek[localTimeInfo.tm_wday],
        localTimeInfo.tm_hour,
        localTimeInfo.tm_min,
        localTimeInfo.tm_sec
    );
    sprintf_s(sendBuf, sizeof(sendBuf), "Content-Length: %d\r\n", strlen(valueDataBuf));
    send(iftttSocket, sendBuf, (int)strlen(sendBuf), 0);

    sprintf_s(sendBuf, sizeof(sendBuf), "Content-Type: application/json\r\n");
    send(iftttSocket, sendBuf, (int)strlen(sendBuf), 0);

    sprintf_s(sendBuf, sizeof(sendBuf), "\r\n");
    send(iftttSocket, sendBuf, (int)strlen(sendBuf), 0);

    send(iftttSocket, valueDataBuf, (int)strlen(valueDataBuf), 0);

    sprintf_s(sendBuf, sizeof(sendBuf), "\r\n");
    send(iftttSocket, sendBuf, (int)strlen(sendBuf), 0);

    /* サーバから受信 */
    while (1) {
        ZeroMemory(recvBuf, BUF_LEN);
        recvSize = recv(iftttSocket, recvBuf, BUF_LEN - 1, 0);
        if (recvSize > 0) {
            _tprintf(_T("%hs\n"), recvBuf);
        }
        else {
            break;
        }
    }

    closesocket(iftttSocket);

    FreeAddrInfo(iftttAddrRes);

    WSACleanup();

    exit(EXIT_SUCCESS);
}

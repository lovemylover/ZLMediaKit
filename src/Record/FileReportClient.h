/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef ZLMEDIAKIT_FILEREPORTCLIENT_H
#define ZLMEDIAKIT_FILEREPORTCLIENT_H

#include "Network/TcpClient.h"
#include "Http/WebSocketClient.h"

namespace mediakit {

class FileReportClient : public toolkit::TcpClient {
public:
    FileReportClient(const toolkit::EventPoller::Ptr &poller = nullptr);
    ~FileReportClient() override;

protected:
    void onRecv(const toolkit::Buffer::Ptr &pBuf) override;
    void onError(const toolkit::SockException &ex) override;
    void onManager() override;
    void onConnect(const toolkit::SockException &ex) override;
    void onFlush() override;

public:
    void setServerInfo(const std::string &host, uint16_t port);

private:
    std::string _host;
    uint16_t _port;
};

// Global WebSocket client instance
extern std::shared_ptr<WebSocketClient<FileReportClient>> g_file_report_client;

} // namespace mediakit

#endif //ZLMEDIAKIT_FILEREPORTCLIENT_H 
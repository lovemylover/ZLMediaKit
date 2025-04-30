/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include "FileReportClient.h"
#include "Util/logger.h"

namespace mediakit {

FileReportClient::FileReportClient(const toolkit::EventPoller::Ptr &poller) : TcpClient(poller) {
    InfoL << "FileReportClient created";
}

FileReportClient::~FileReportClient() {
    InfoL << "FileReportClient destroyed";
}

void FileReportClient::onRecv(const toolkit::Buffer::Ptr &pBuf) {
    DebugL << "Received response: " << pBuf->toString();
}

void FileReportClient::onError(const toolkit::SockException &ex) {
    WarnL << "WebSocket connection error: " << ex;
    // Try to reconnect after 5 seconds
    getPoller()->doDelayTask(5000, [this]() {
        startConnect(_host, _port);
        return 0;
    });
}

void FileReportClient::onManager() {
    // Send heartbeat every 30 seconds
    SockSender::send("{\"type\":\"heartbeat\"}");
}

void FileReportClient::onConnect(const toolkit::SockException &ex) {
    if (ex) {
        WarnL << "Failed to connect to WebSocket server: " << ex;
        return;
    }
    InfoL << "Connected to WebSocket server";
}

void FileReportClient::onFlush() {
    DebugL << "All data sent";
}

void FileReportClient::setServerInfo(const std::string &host, uint16_t port) {
    _host = host;
    _port = port;
}

// Initialize global WebSocket client instance
std::shared_ptr<WebSocketClient<FileReportClient>> g_file_report_client;

} // namespace mediakit 
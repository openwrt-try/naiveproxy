// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/test_proxy_delegate.h"

#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/proxy_resolution/proxy_info.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

TestProxyDelegate::TestProxyDelegate() = default;

TestProxyDelegate::~TestProxyDelegate() = default;

void TestProxyDelegate::VerifyOnHttp1TunnelHeadersReceived(
    const ProxyServer& proxy_server,
    const std::string& response_header_name,
    const std::string& response_header_value) const {
  EXPECT_EQ(proxy_server, on_tunnel_headers_received_proxy_server_);
  ASSERT_NE(on_tunnel_headers_received_headers_.get(), nullptr);
  EXPECT_TRUE(on_tunnel_headers_received_headers_->HasHeaderValue(
      response_header_name, response_header_value));
}

void TestProxyDelegate::OnResolveProxy(
    const GURL& url,
    const std::string& method,
    const ProxyRetryInfoMap& proxy_retry_info,
    ProxyInfo* result) {
  if (trusted_spdy_proxy_.is_valid()) {
    ProxyList new_proxy_list;
    for (const auto& proxy_server : result->proxy_list().GetAll()) {
      if (proxy_server == trusted_spdy_proxy_) {
        new_proxy_list.AddProxyServer(ProxyServer(
            proxy_server.scheme(), proxy_server.host_port_pair(), true));
      } else {
        new_proxy_list.AddProxyServer(proxy_server);
      }
    }
    result->UseProxyList(new_proxy_list);
    result->set_traffic_annotation(
        MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));
  }

  // Only set |alternative_proxy_server_| as the alternative proxy if the
  // ProxyService has not marked it as bad.
  ProxyInfo alternative_proxy_info;
  alternative_proxy_info.UseProxyServer(alternative_proxy_server_);
  alternative_proxy_info.DeprioritizeBadProxies(proxy_retry_info);
  if (!alternative_proxy_info.is_empty())
    result->SetAlternativeProxy(alternative_proxy_info.proxy_server());
  result->set_traffic_annotation(
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));
}

void TestProxyDelegate::OnFallback(const ProxyServer& bad_proxy,
                                   int net_error) {}

void TestProxyDelegate::OnBeforeHttp1TunnelRequest(
    const ProxyServer& proxy_server,
    HttpRequestHeaders* extra_headers) {
  on_before_tunnel_request_called_ = true;
  if (extra_headers)
    extra_headers->SetHeader("Foo", proxy_server.ToURI());
}

Error TestProxyDelegate::OnHttp1TunnelHeadersReceived(
    const ProxyServer& proxy_server,
    const HttpResponseHeaders& response_headers) {
  EXPECT_EQ(on_tunnel_headers_received_headers_.get(), nullptr);
  on_tunnel_headers_received_headers_ =
      base::MakeRefCounted<HttpResponseHeaders>(response_headers.raw_headers());

  on_tunnel_headers_received_proxy_server_ = proxy_server;
  return OK;
}

}  // namespace net

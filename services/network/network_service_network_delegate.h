// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_NETWORK_SERVICE_NETWORK_DELEGATE_H_
#define SERVICES_NETWORK_NETWORK_SERVICE_NETWORK_DELEGATE_H_

#include "base/component_export.h"
#include "base/macros.h"
#include "net/base/network_delegate_impl.h"
#include "components/prefs/pref_member.h"

namespace net {
class URLRequest;
namespace blockers {
class BlockersWorker;
class ShieldsConfig;
}
}

namespace network {

struct OnBeforeURLRequestContext;

class NetworkContext;

class COMPONENT_EXPORT(NETWORK_SERVICE) NetworkServiceNetworkDelegate
    : public net::NetworkDelegateImpl {
 public:
  // |network_context| is guaranteed to outlive this class.
  explicit NetworkServiceNetworkDelegate(NetworkContext* network_context);
  ~NetworkServiceNetworkDelegate() override;

 private:
  // net::NetworkDelegateImpl implementation.
  void OnBeforeSendHeaders(net::URLRequest* request,
                           const net::ProxyInfo& proxy_info,
                           const net::ProxyRetryInfoMap& proxy_retry_info,
                           net::HttpRequestHeaders* headers) override;
  int OnBeforeStartTransaction(net::URLRequest* request,
                               net::CompletionOnceCallback callback,
                               net::HttpRequestHeaders* headers) override;
  int OnHeadersReceived(
      net::URLRequest* request,
      net::CompletionOnceCallback callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) override;
  bool OnCanGetCookies(const net::URLRequest& request,
                       const net::CookieList& cookie_list,
                       bool allowed_from_caller) override;
  bool OnCanSetCookie(const net::URLRequest& request,
                      const net::CanonicalCookie& cookie,
                      net::CookieOptions* options,
                      bool allowed_from_caller) override;
  bool OnCanAccessFile(const net::URLRequest& request,
                       const base::FilePath& original_path,
                       const base::FilePath& absolute_path) const override;

  bool OnCanQueueReportingReport(const url::Origin& origin) const override;
  void OnCanSendReportingReports(std::set<url::Origin> origins,
                                 base::OnceCallback<void(std::set<url::Origin>)>
                                     result_callback) const override;
  bool OnCanSetReportingClient(const url::Origin& origin,
                               const GURL& endpoint) const override;
  bool OnCanUseReportingClient(const url::Origin& origin,
                               const GURL& endpoint) const override;

  int OnBeforeURLRequest(net::URLRequest* request,
                         net::CompletionOnceCallback callback,
                         GURL* new_url,
                         bool call_callback) override;
  int OnBeforeURLRequest_PreBlockersWork(
            net::URLRequest* request,
            net::CompletionOnceCallback callback,
            GURL* new_url,
            std::shared_ptr<OnBeforeURLRequestContext> ctx);
  int OnBeforeURLRequest_AdBlockPreFileWork(
            net::URLRequest* request,
            net::CompletionOnceCallback callback,
            GURL* new_url,
            std::shared_ptr<OnBeforeURLRequestContext> ctx);
  void OnBeforeURLRequest_AdBlockFileWork(std::shared_ptr<OnBeforeURLRequestContext> ctx);
  int OnBeforeURLRequest_AdBlockPostFileWork(
            net::URLRequest* request,
            net::CompletionOnceCallback callback,
            GURL* new_url,
            std::shared_ptr<OnBeforeURLRequestContext> ctx);
  int OnBeforeURLRequest_HttpsePreFileWork(
            net::URLRequest* request,
            net::CompletionOnceCallback callback,
            GURL* new_url,
            std::shared_ptr<OnBeforeURLRequestContext> ctx);
  void OnBeforeURLRequest_HttpseFileWork(
            net::URLRequest* request,
            std::shared_ptr<OnBeforeURLRequestContext> ctx);
  int OnBeforeURLRequest_HttpsePostFileWork(
            net::URLRequest* request,
            net::CompletionOnceCallback callback,
            GURL* new_url,
            std::shared_ptr<OnBeforeURLRequestContext> ctx);
  int OnBeforeURLRequest_PostBlockers(
            net::URLRequest* request,
            net::CompletionOnceCallback callback,
            GURL* new_url,
            std::shared_ptr<OnBeforeURLRequestContext> ctx);

  int HandleClearSiteDataHeader(
      net::URLRequest* request,
      net::CompletionOnceCallback callback,
      const net::HttpResponseHeaders* original_response_headers);

  void FinishedClearSiteData(base::WeakPtr<net::URLRequest> request,
                             net::CompletionOnceCallback callback);
  void FinishedCanSendReportingReports(
      base::OnceCallback<void(std::set<url::Origin>)> result_callback,
      const std::vector<url::Origin>& origins);

  NetworkContext* network_context_;

  std::shared_ptr<net::blockers::BlockersWorker> blockers_worker_;
  // (TODO)find a better way to handle last first party
  GURL last_first_party_url_;

  mutable base::WeakPtrFactory<NetworkServiceNetworkDelegate> weak_ptr_factory_{
      this};

  DISALLOW_COPY_AND_ASSIGN(NetworkServiceNetworkDelegate);
};

}  // namespace network

#endif  // SERVICES_NETWORK_NETWORK_SERVICE_NETWORK_DELEGATE_H_

/**
 * @file curl_wrapper.cpp
 * @author The FOV Team
 * @brief Simple c++ CURL wrapper
 * @version 0.1
 * @date 2026-02-07
 */

#include <cstring>

#include "curl_wrapper.hpp"

SimpleCurlRequest::SimpleCurlRequest(const std::string &url, httpMethod method)
	: curl(nullptr),
	  requestHeaders(nullptr),
	  requestResult(-1),
	  responseContent(""),
	  payload(""),
	  method(method)
{
	curl = curl_easy_init();
	if (!curl) {
		throw SimpleCurlException("Failed to initialize CURL handle");
	}

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseContent);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	setHttpMethod(method);
}

SimpleCurlRequest::~SimpleCurlRequest()
{
	if (requestHeaders != nullptr) {
		curl_slist_free_all(requestHeaders);
	}
	if (curl != nullptr) {
		curl_easy_cleanup(curl);
	}
}

void SimpleCurlRequest::setHttpMethod(httpMethod method)
{
	this->method = method;
	if (method == HTTP_POST) {
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
	} else {
		curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	}
}

void SimpleCurlRequest::setHeaders(std::vector<std::string> &headers)
{
	if (requestHeaders != nullptr) {
		curl_slist_free_all(requestHeaders);
		requestHeaders = nullptr;
	}

	for (const auto &element : headers) {
		requestHeaders = curl_slist_append(requestHeaders, element.c_str());
		if (requestHeaders == nullptr) {
			throw SimpleCurlException("setHeaders failed to allocate memory for curl request header");
		}
	}
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, requestHeaders);
}

void SimpleCurlRequest::setRequestPayload(const std::string &payload)
{
	this->payload = payload;
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, this->payload.c_str());
}

int SimpleCurlRequest::performRequest(int timeout)
{
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)this->payload.size());

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		throw SimpleCurlException(res);
	} else {
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &requestResult);
		return (requestResult);
	}
}

int SimpleCurlRequest::getResponseCode(void) const
{
	return requestResult;
}

std::string SimpleCurlRequest::getResponseContent(void) const
{
	return responseContent;
}

size_t SimpleCurlRequest::writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	std::string *mem = static_cast<std::string *>(userp);
	mem->append(static_cast<char *>(contents), realsize);
	return realsize;
}

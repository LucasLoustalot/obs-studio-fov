/**
 * @file curl_wrapper.cpp
 * @author The FOV Team
 * @brief Implementation of the SimpleCurlRequest class managing synchronous HTTP operations via libcurl.
 * @version 0.1
 * @date 2026-02-07
 */

#include <cstring>

#include "curl_wrapper.hpp"

/**
 * @brief Construct a new SimpleCurlRequest object.
 * @param[in] url String containing the target URL.
 * @param[in] method HTTP method enum value (e.g., HTTP_GET, HTTP_POST).
 * @throws SimpleCurlException If curl_easy_init fails to allocate the internal CURL handle.
 */
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

/**
 * @brief Destroy the SimpleCurlRequest object and free allocated resources.
 */
SimpleCurlRequest::~SimpleCurlRequest()
{
	if (requestHeaders != nullptr) {
		curl_slist_free_all(requestHeaders);
	}
	if (curl != nullptr) {
		curl_easy_cleanup(curl);
	}
}

/**
 * @brief Set the HTTP method for the request.
 * @param[in] method HTTP method enum value (e.g., HTTP_GET, HTTP_POST).
 */
void SimpleCurlRequest::setHttpMethod(httpMethod method)
{
	this->method = method;
	if (method == HTTP_POST) {
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
	} else {
		curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	}
}

/**
 * @brief Set the HTTP headers for the request.
 * @param[in] headers Vector of strings containing the HTTP headers.
 * @throws SimpleCurlException If curl_slist_append fails to allocate memory for the header list.
 */
void SimpleCurlRequest::setHeaders(const std::vector<std::string> &headers)
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

/**
 * @brief Set the payload data for an HTTP POST request.
 * @param[in] payload String containing the request payload data.
 */
void SimpleCurlRequest::setRequestPayload(const std::string &payload)
{
	this->payload = payload;
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, this->payload.c_str());
}

/**
 * @brief Execute the configured HTTP request.
 * @param[in] timeout Request timeout duration in seconds.
 * @return int HTTP response code on success.
 * @throws SimpleCurlException If curl_easy_perform returns an error code other than CURLE_OK.
 */
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

/**
 * @brief Get the HTTP response code from the executed request.
 * @return int HTTP response code.
 */
int SimpleCurlRequest::getResponseCode(void) const
{
	return requestResult;
}

/**
 * @brief Get the response body content from the executed request.
 * @return std::string String containing the raw response body.
 */
std::string SimpleCurlRequest::getResponseContent(void) const
{
	return responseContent;
}

/**
 * @brief Callback function used by CURL to write incoming data chunks.
 * @param[in] contents Pointer to the incoming data chunk.
 * @param[in] size Size of an individual data element.
 * @param[in] nmemb Number of data elements.
 * @param[in,out] userp Pointer to the destination string object.
 * @return size_t Total number of bytes processed.
 */
size_t SimpleCurlRequest::writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	std::string *mem = static_cast<std::string *>(userp);
	mem->append(static_cast<char *>(contents), realsize);
	return realsize;
}

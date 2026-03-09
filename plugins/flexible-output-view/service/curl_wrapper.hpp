/**
 * @file curl_wrapper.hpp
 * @author The FOV Team
 * @brief Simple c++ CURL wrapper
 * @version 0.1
 * @date 2026-02-07
 */

#pragma once

#include <util/curl/curl-helper.h>

#include <vector>
#include <exception>
#include <string>

class SimpleCurlException : public std::exception {
private:
	std::string message;
	CURLcode curlCode;

public:
	SimpleCurlException(CURLcode code) : curlCode(code)
	{
		message = "CURL Error (" + std::to_string(code) + "): " + curl_easy_strerror(code);
	}

	SimpleCurlException(const std::string &customMsg)
		: message("SimpleCurl Internal: " + customMsg),
		  curlCode(CURLE_OK)
	{
	}

	virtual const char *what() const noexcept override { return message.c_str(); }
	CURLcode getCode() const { return curlCode; }
};

class SimpleCurlRequest {
public:
	enum httpMethod { HTTP_GET, HTTP_POST };

    SimpleCurlRequest() = delete;
    SimpleCurlRequest(const SimpleCurlRequest&) = delete;
    SimpleCurlRequest &operator=(SimpleCurlRequest &) = delete;

	explicit SimpleCurlRequest(const std::string &url, httpMethod method = HTTP_GET);
	~SimpleCurlRequest();

	void setHttpMethod(httpMethod method);
	void setHeaders(const std::vector<std::string> &headers);
	void setRequestPayload(const std::string &payload);

	int performRequest(int timeout = 5);

	int getResponseCode(void) const;
	std::string getResponseContent(void) const;

private:
	CURL *curl;
	struct curl_slist *requestHeaders;
	int requestResult;
	std::string responseContent;
	std::string payload;

	httpMethod method;

	static size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp);
};

/**
 * @file curl_wrapper.hpp
 * @author The FOV Team
 * @brief Header file for the SimpleCurlRequest class managing synchronous HTTP operations via libcurl.
 * @version 0.1
 * @date 2026-02-07
 */

#pragma once

#include <util/curl/curl-helper.h>

#include <vector>
#include <exception>
#include <string>

/**
 * @class SimpleCurlException
 * @brief Custom exception class for libcurl and internal transport layer errors.
 */
class SimpleCurlException : public std::exception {
private:
	std::string message; /**< Built error message string. */
	CURLcode curlCode;   /**< Native libcurl error code context. */

public:
	/**
     * @brief Construct a new SimpleCurlException object from a CURLcode.
     * @param[in] code The native libcurl error code.
     */
	SimpleCurlException(CURLcode code) : curlCode(code)
	{
		message = "CURL Error (" + std::to_string(code) + "): " + curl_easy_strerror(code);
	}

	/**
     * @brief Construct a new SimpleCurlException object with a custom error message.
     * @param[in] customMsg String containing the custom internal error description.
     */
	SimpleCurlException(const std::string &customMsg)
		: message("SimpleCurl Internal: " + customMsg),
		  curlCode(CURLE_OK)
	{
	}

	/**
     * @brief Get the explanatory string for the exception.
     * @return const char* Pointer to a null-terminated string containing the error message.
     */
	virtual const char *what() const noexcept override { return message.c_str(); }

	/**
     * @brief Get the underlying libcurl error code associated with this exception.
     * @return CURLcode The native libcurl error code. Returns CURLE_OK if it is an internal error.
     */
	CURLcode getCode() const { return curlCode; }
};

/**
 * @class SimpleCurlRequest
 * @brief Class managing synchronous HTTP operations via libcurl.
 */
class SimpleCurlRequest {
public:
	/**
     * @enum httpMethod
     * @brief Supported HTTP method variants.
     */
	enum httpMethod {
		HTTP_GET, /**< HTTP GET request method. */
		HTTP_POST /**< HTTP POST request method. */
	};

	SimpleCurlRequest() = delete;
	SimpleCurlRequest(const SimpleCurlRequest &) = delete;
	SimpleCurlRequest &operator=(SimpleCurlRequest &) = delete;

	/**
     * @brief Construct a new SimpleCurlRequest object.
     * @param[in] url String containing the target URL.
     * @param[in] method HTTP method enum value (e.g., HTTP_GET, HTTP_POST). Default is HTTP_GET.
     * @throws SimpleCurlException If curl_easy_init fails to allocate the internal CURL handle.
     */
	explicit SimpleCurlRequest(const std::string &url, httpMethod method = HTTP_GET);

	/**
     * @brief Destroy the SimpleCurlRequest object and free allocated resources.
     */
	~SimpleCurlRequest();

	/**
     * @brief Set the HTTP method for the request.
     * @param[in] method HTTP method enum value (e.g., HTTP_GET, HTTP_POST).
     */
	void setHttpMethod(httpMethod method);

	/**
     * @brief Set the HTTP headers for the request.
     * @param[in] headers Vector of strings containing the HTTP headers.
     * @throws SimpleCurlException If curl_slist_append fails to allocate memory for the header list.
     */
	void setHeaders(const std::vector<std::string> &headers);

	/**
     * @brief Set the payload data for an HTTP POST request.
     * @param[in] payload String containing the request payload data.
     */
	void setRequestPayload(const std::string &payload);

	/**
     * @brief Execute the configured HTTP request.
     * @param[in] timeout Request timeout duration in seconds. Default value is 5.
     * @return int HTTP response code on success.
     * @throws SimpleCurlException If curl_easy_perform returns an error code other than CURLE_OK.
     */
	int performRequest(int timeout = 5);

	/**
     * @brief Get the HTTP response code from the executed request.
     * @return int HTTP response code.
     */
	int getResponseCode(void) const;

	/**
     * @brief Get the response body content from the executed request.
     * @return std::string String containing the raw response body.
     */
	std::string getResponseContent(void) const;

private:
	CURL *curl;                        /**< Pointer to the internal CURL easy handle. */
	struct curl_slist *requestHeaders; /**< Pointer to the linked list storing HTTP request headers. */
	int requestResult;                 /**< Cached HTTP response code from the last request execution. */
	std::string responseContent;       /**< String containing the accumulated HTTP response body. */
	std::string payload;               /**< String containing the raw outbound HTTP POST payload data. */

	httpMethod method; /**< Configured HTTP method type for the next execution. */

	/**
     * @brief Callback function used by CURL to write incoming data chunks.
     * @param[in] contents Pointer to the incoming data chunk.
     * @param[in] size Size of an individual data element.
     * @param[in] nmemb Number of data elements.
     * @param[in,out] userp Pointer to the destination string object.
     * @return size_t Total number of bytes processed.
     */
	static size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp);
};

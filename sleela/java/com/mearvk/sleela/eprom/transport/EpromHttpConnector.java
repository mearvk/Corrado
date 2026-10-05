package com.mearvk.sleela.eprom.transport;

import com.mearvk.sleela.eprom.connector.EpromConnector;
import com.mearvk.sleela.eprom.connector.EpromInvocation;
import com.mearvk.sleela.eprom.connector.EpromResult;

import java.net.URI;
import java.net.URLEncoder;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.charset.StandardCharsets;
import java.time.Duration;
import java.util.Objects;

/**
 * Java HTTP client for an {@link EpromHttpGateway}. Implements the same
 * {@link EpromConnector} contract as the in-process transport, so a host can
 * drive a remote programmer with identical call sites. Modelled on SLeeLa's
 * {@code SleelaHttpConnector}.
 *
 * <p>Talks to {@code <base>/invoke?operation=<name>} (POST, body = arguments)
 * and {@code <base>/health} (GET), both UTF-8 text. A response size cap bounds
 * client memory. Faults are folded into {@link EpromResult#failure(String)}.
 */
public final class EpromHttpConnector implements EpromConnector {

    private final HttpClient client;
    private final URI baseUri;
    private final Duration timeout;
    private final int maxResponseBytes;

    public EpromHttpConnector(URI baseUri) {
        this(baseUri, HttpClient.newHttpClient(), Duration.ofSeconds(30), 1 << 20);
    }

    public EpromHttpConnector(URI baseUri, HttpClient client, Duration timeout, int maxResponseBytes) {
        this.baseUri = normalize(Objects.requireNonNull(baseUri, "baseUri"));
        this.client = Objects.requireNonNull(client, "client");
        this.timeout = Objects.requireNonNull(timeout, "timeout");
        if (maxResponseBytes < 1024) throw new IllegalArgumentException("maxResponseBytes < 1024");
        this.maxResponseBytes = maxResponseBytes;
    }

    @Override
    public EpromResult invoke(EpromInvocation invocation) {
        try {
            URI uri = baseUri.resolve("invoke?operation=" +
                    URLEncoder.encode(invocation.operation(), StandardCharsets.UTF_8));
            HttpRequest request = HttpRequest.newBuilder(uri)
                    .timeout(timeout)
                    .header("Content-Type", "text/plain; charset=utf-8")
                    .header("Accept", "text/plain")
                    .POST(HttpRequest.BodyPublishers.ofString(
                            invocation.arguments(), StandardCharsets.UTF_8))
                    .build();
            HttpResponse<byte[]> response =
                    client.send(request, HttpResponse.BodyHandlers.ofByteArray());
            if (response.body().length > maxResponseBytes) {
                return EpromResult.failure(
                        "HTTP response exceeds configured limit of " + maxResponseBytes + " bytes");
            }
            String body = new String(response.body(), StandardCharsets.UTF_8);
            int family = response.statusCode() / 100;
            if (family == 2) {
                return EpromResult.success(body);
            }
            // The gateway reports an operation-level fault as 422; map it (and
            // any other non-2xx) to a connector failure carrying the message.
            return EpromResult.failure("HTTP " + response.statusCode() + ": " + body);
        } catch (Exception failure) {
            return EpromResult.failure(messageOf(failure));
        }
    }

    @Override
    public String health() throws Exception {
        HttpRequest request = HttpRequest.newBuilder(baseUri.resolve("health"))
                .timeout(timeout)
                .header("Accept", "text/plain")
                .GET()
                .build();
        HttpResponse<byte[]> response =
                client.send(request, HttpResponse.BodyHandlers.ofByteArray());
        if (response.body().length > maxResponseBytes) {
            throw new IllegalStateException(
                    "HTTP health response exceeds configured limit of " + maxResponseBytes + " bytes");
        }
        String body = new String(response.body(), StandardCharsets.UTF_8);
        if (response.statusCode() / 100 != 2) {
            throw new IllegalStateException("HTTP " + response.statusCode() + ": " + body);
        }
        return body;
    }

    private static URI normalize(URI uri) {
        String text = uri.toString();
        return URI.create(text.endsWith("/") ? text : text + "/");
    }

    private static String messageOf(Throwable failure) {
        return failure.getMessage() == null ? failure.toString() : failure.getMessage();
    }
}

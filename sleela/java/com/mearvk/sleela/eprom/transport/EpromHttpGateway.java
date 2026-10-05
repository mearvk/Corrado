package com.mearvk.sleela.eprom.transport;

import com.mearvk.sleela.eprom.connector.EpromConnector;
import com.mearvk.sleela.eprom.connector.EpromInvocation;
import com.mearvk.sleela.eprom.connector.EpromResult;

import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpServer;

import java.io.IOException;
import java.io.InputStream;
import java.net.InetSocketAddress;
import java.net.URI;
import java.nio.charset.StandardCharsets;
import java.util.Objects;

/**
 * Small JDK-only HTTP gateway exposing an {@link EpromConnector} over HTTP, for
 * web and browser-driven clients. Modelled directly on SLeeLa's
 * {@code SleelaHttpGateway} (see the SLeeLa repo's {@code CONNECTOR.md}).
 *
 * <p>Endpoints:
 * <ul>
 *   <li>{@code GET  /eprom/health}</li>
 *   <li>{@code POST /eprom/invoke?operation=<name>} — body is the argument string</li>
 * </ul>
 *
 * <p>The request body is the operation's argument string (e.g. a path); the
 * response is UTF-8 text. No Java serialization is exposed. A delegate
 * {@link EpromConnector} (e.g. {@link DirectEpromConnector}) remains
 * authoritative for the actual chip operation.
 *
 * <p><b>Safety:</b> this is an integration boundary, not an authorization
 * system. A real deployment must add authentication, TLS, a narrow CORS origin,
 * and rate/size limits in front of it — and the Corrado hardware caution still
 * applies (back up stock first, verify every write). Exposing destructive verbs
 * ({@code write}, {@code erase}) over an unauthenticated network is unsafe.
 */
public final class EpromHttpGateway implements AutoCloseable {

    /** Hard cap on an accepted request body, to bound memory on a POST. */
    private static final int MAX_BODY_BYTES = 1 << 20; // 1 MiB

    private final HttpServer server;
    private final EpromConnector delegate;
    private final String allowOrigin;

    public EpromHttpGateway(String host, int port, EpromConnector delegate) throws IOException {
        this(host, port, delegate, "");
    }

    public EpromHttpGateway(String host, int port, EpromConnector delegate, String allowOrigin)
            throws IOException {
        Objects.requireNonNull(host, "host");
        this.delegate = Objects.requireNonNull(delegate, "delegate");
        this.allowOrigin = allowOrigin == null ? "" : allowOrigin;
        this.server = HttpServer.create(new InetSocketAddress(host, port), 0);
        this.server.createContext("/eprom/health", this::health);
        this.server.createContext("/eprom/invoke", this::invoke);
    }

    public void start() { server.start(); }

    public int port() { return server.getAddress().getPort(); }

    public URI healthUri() {
        return URI.create("http://" + server.getAddress().getHostString()
                + ":" + port() + "/eprom/health");
    }

    public URI baseUri() {
        return URI.create("http://" + server.getAddress().getHostString()
                + ":" + port() + "/eprom/");
    }

    private void health(HttpExchange exchange) throws IOException {
        if (!"GET".equalsIgnoreCase(exchange.getRequestMethod())) {
            send(exchange, 405, "GET required");
            return;
        }
        try {
            send(exchange, 200, delegate.health());
        } catch (Exception failure) {
            send(exchange, 503, messageOf(failure));
        }
    }

    private void invoke(HttpExchange exchange) throws IOException {
        if ("OPTIONS".equalsIgnoreCase(exchange.getRequestMethod())) {
            addCors(exchange);
            send(exchange, 204, "");
            return;
        }
        if (!"POST".equalsIgnoreCase(exchange.getRequestMethod())) {
            send(exchange, 405, "POST required");
            return;
        }

        String operation = queryParameter(exchange.getRequestURI(), "operation");
        if (operation == null || operation.isBlank()) {
            send(exchange, 400, "operation is required");
            return;
        }

        String arguments;
        try (InputStream input = exchange.getRequestBody()) {
            byte[] body = readBounded(input, MAX_BODY_BYTES);
            if (body == null) {
                send(exchange, 413, "request body exceeds " + MAX_BODY_BYTES + " bytes");
                return;
            }
            arguments = new String(body, StandardCharsets.UTF_8);
        }

        try {
            EpromResult result = delegate.invoke(new EpromInvocation(operation, arguments));
            if (result.success()) {
                send(exchange, 200, result.value());
            } else {
                // Operation ran but reported a fault -> 422 with the error text.
                send(exchange, 422, result.error());
            }
        } catch (Throwable failure) {
            // Never let a fault escape the handler without a status.
            send(exchange, 500, messageOf(failure));
        }
    }

    /** Reads at most {@code limit} bytes; null if the stream exceeds the cap. */
    private static byte[] readBounded(InputStream input, int limit) throws IOException {
        java.io.ByteArrayOutputStream acc = new java.io.ByteArrayOutputStream();
        byte[] buf = new byte[8192];
        int total = 0, n;
        while ((n = input.read(buf)) != -1) {
            total += n;
            if (total > limit) return null;
            acc.write(buf, 0, n);
        }
        return acc.toByteArray();
    }

    private void send(HttpExchange exchange, int status, String body) throws IOException {
        addCors(exchange);
        byte[] bytes = body.getBytes(StandardCharsets.UTF_8);
        exchange.getResponseHeaders().set("Content-Type", "text/plain; charset=utf-8");
        exchange.sendResponseHeaders(status, bytes.length == 0 ? -1 : bytes.length);
        if (bytes.length > 0) {
            try (var output = exchange.getResponseBody()) { output.write(bytes); }
        } else {
            exchange.close();
        }
    }

    private void addCors(HttpExchange exchange) {
        if (!allowOrigin.isBlank()) {
            exchange.getResponseHeaders().set("Access-Control-Allow-Origin", allowOrigin);
            exchange.getResponseHeaders().set("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
            exchange.getResponseHeaders().set("Access-Control-Allow-Headers", "Content-Type");
        }
    }

    private static String queryParameter(URI uri, String name) {
        String query = uri.getRawQuery();
        if (query == null) return null;
        for (String part : query.split("&")) {
            int sep = part.indexOf('=');
            String key = sep < 0 ? part : part.substring(0, sep);
            if (name.equals(java.net.URLDecoder.decode(key, StandardCharsets.UTF_8))) {
                String value = sep < 0 ? "" : part.substring(sep + 1);
                return java.net.URLDecoder.decode(value, StandardCharsets.UTF_8);
            }
        }
        return null;
    }

    private static String messageOf(Throwable failure) {
        return failure.getMessage() == null ? failure.toString() : failure.getMessage();
    }

    @Override
    public void close() { server.stop(0); }
}

import java.io.*;
import java.net.*;
import java.nio.file.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.regex.*;
import java.util.stream.Collectors;
import java.nio.charset.StandardCharsets;
import org.json.*;

public class PhoneDirectoryServer {
    // Server configuration
    private static final int PORT = 8080;
    private static final String UPLOAD_FOLDER = "uploads";
    private static final String DATA_FILE = "phone_directory.json";
    private static final int MAX_CONTENT_LENGTH = 10 * 1024 * 1024;

    // MIME types
    private static final Map<String, String> MIME_TYPES = new HashMap<>();
    static {
        MIME_TYPES.put("html", "text/html");
        MIME_TYPES.put("css", "text/css");
        MIME_TYPES.put("js", "application/javascript");
        MIME_TYPES.put("json", "application/json");
        MIME_TYPES.put("png", "image/png");
        MIME_TYPES.put("jpg", "image/jpeg");
        MIME_TYPES.put("jpeg", "image/jpeg");
        MIME_TYPES.put("gif", "image/gif");
    }

    public static void main(String[] args) {
        // Start the server
        try (ServerSocket serverSocket = new ServerSocket(PORT)) {
            System.out.println("Phone Directory Server started on port " + PORT);
            
            // Thread pool for handling multiple clients
            ExecutorService executor = Executors.newFixedThreadPool(10);
            
            while (true) {
                try {
                    Socket clientSocket = serverSocket.accept();
                    executor.submit(() -> handleClient(clientSocket));
                } catch (IOException e) {
                    System.err.println("Error accepting client connection: " + e.getMessage());
                }
            }
        } catch (IOException e) {
            System.err.println("Server error: " + e.getMessage());
        }
    }

    private static void serveApiSearchContacts(Map<String, String> queryParams, OutputStream out) throws IOException {
        String query = queryParams.getOrDefault("q", "").toLowerCase();
        JSONObject allContacts = loadData();
        JSONObject filteredContacts = new JSONObject();
        
        if (!query.isEmpty()) {
            for (String name : allContacts.keySet()) {
                JSONObject contact = allContacts.getJSONObject(name);
                String phone = contact.getString("phone");
                
                if (name.toLowerCase().contains(query) || phone.contains(query)) {
                    JSONObject simplified = new JSONObject();
                    simplified.put("phone", contact.getString("phone"));
                    simplified.put("has_photo", contact.has("photo") && !contact.getString("photo").isEmpty());
                    filteredContacts.put(name, simplified);
                }
            }
        }
        
        sendResponse(out, 200, "OK", "application/json", filteredContacts.toString(2).getBytes());
    }
    
    // Then modify the handleGetRequest method to include the new search endpoint
    private static void handleGetRequest(String path, Map<String, String> headers, OutputStream out) throws IOException {
        Map<String, String> queryParams = new HashMap<>();
        int queryStart = path.indexOf('?');
        if (queryStart != -1) {
            String queryString = path.substring(queryStart + 1);
            path = path.substring(0, queryStart);
            
            String[] params = queryString.split("&");
            for (String param : params) {
                String[] keyValue = param.split("=", 2);
                if (keyValue.length == 2) {
                    queryParams.put(keyValue[0], urlDecode(keyValue[1]));
                }
            }
        }
    
        // Route the request
        if (path.equals("/") || path.equals("/index.html")) {
            serveIndexPage(out);
        } else if (path.equals("/add") || path.equals("/add.html")) {
            serveAddPage(out);
        } else if (path.equals("/search") || path.equals("/search.html")) {
            serveSearchPage(queryParams, out);
        } else if (path.startsWith("/edit/")) {
            String name = path.substring("/edit/".length());
            serveEditPage(urlDecode(name), out);
        } else if (path.startsWith("/delete/")) {
            String name = path.substring("/delete/".length());
            serveDeletePage(urlDecode(name), out);
        } else if (path.startsWith("/photo/")) {
            String name = path.substring("/photo/".length());
            servePhoto(urlDecode(name), out);
        } else if (path.equals("/api/contacts")) {
            if (queryParams.containsKey("q")) {
                serveApiSearchContacts(queryParams, out);
            } else {
                serveApiContacts(out);
            }
        } else if (path.startsWith("/api/contacts/")) {
            String name = path.substring("/api/contacts/".length());
            serveApiContact(urlDecode(name), out);
        } else if (path.startsWith("/static/")) {
            // Serve static files (new addition)
            serveStaticFile(path, out);
        } else {
            // Return 404 Not Found
            serveErrorPage(out, 404, "Not Found", "The requested resource does not exist.");
        }
    }
    
    // Add method to serve static files
    private static void serveStaticFile(String path, OutputStream out) throws IOException {
        try {
            Path filePath = Paths.get("." + path);
            if (!Files.exists(filePath)) {
                serveErrorPage(out, 404, "Not Found", "The requested file does not exist.");
                return;
            }
            
            // Get file extension for MIME type
            String fileName = filePath.getFileName().toString();
            String extension = "";
            int dotIndex = fileName.lastIndexOf('.');
            if (dotIndex > 0) {
                extension = fileName.substring(dotIndex + 1).toLowerCase();
            }
            
            String mimeType = MIME_TYPES.getOrDefault(extension, "application/octet-stream");
            byte[] fileData = Files.readAllBytes(filePath);
            
            sendResponse(out, 200, "OK", mimeType, fileData);
        } catch (IOException e) {
            serveErrorPage(out, 500, "Internal Server Error", "Failed to read the requested file.");
        }
    }

    // Page rendering methods
    private static void serveIndexPage(OutputStream out) throws IOException {
        JSONObject contacts = loadData();
        String content = renderTemplate("templates/index.html", Map.of("contacts", contacts));
        sendResponse(out, 200, "OK", "text/html", content.getBytes());
    }

    private static void serveAddPage(OutputStream out) throws IOException {
        String content = renderTemplate("templates/add.html", Map.of());
        sendResponse(out, 200, "OK", "text/html", content.getBytes());
    }

    private static void serveSearchPage(Map<String, String> queryParams, OutputStream out) throws IOException {
        String query = queryParams.getOrDefault("q", "").toLowerCase();
        JSONObject allContacts = loadData();
        JSONObject filteredContacts = new JSONObject();
        
        if (!query.isEmpty()) {
            for (String name : allContacts.keySet()) {
                JSONObject contact = allContacts.getJSONObject(name);
                String phone = contact.getString("phone");
                
                if (name.toLowerCase().contains(query) || phone.contains(query)) {
                    filteredContacts.put(name, contact);
                }
            }
        }
        
        Map<String, Object> templateVars = new HashMap<>();
        templateVars.put("contacts", filteredContacts);
        templateVars.put("query", query);
        
        String content = renderTemplate("templates/search.html", templateVars);
        sendResponse(out, 200, "OK", "text/html", content.getBytes());
    }

    private static void serveEditPage(String name, OutputStream out) throws IOException {
        JSONObject contacts = loadData();
        
        if (!contacts.has(name)) {
            serveErrorPage(out, 404, "Not Found", "Contact not found.");
            return;
        }
        
        Map<String, Object> templateVars = new HashMap<>();
        templateVars.put("name", name);
        templateVars.put("contact", contacts.getJSONObject(name));
        
        String content = renderTemplate("templates/edit.html", templateVars);
        sendResponse(out, 200, "OK", "text/html", content.getBytes());
    }

    private static void serveDeletePage(String name, OutputStream out) throws IOException {
        JSONObject contacts = loadData();
        
        if (!contacts.has(name)) {
            serveErrorPage(out, 404, "Not Found", "Contact not found.");
            return;
        }
        
        Map<String, Object> templateVars = new HashMap<>();
        templateVars.put("name", name);
        templateVars.put("contact", contacts.getJSONObject(name));
        
        String content = renderTemplate("templates/delete.html", templateVars);
        sendResponse(out, 200, "OK", "text/html", content.getBytes());
    }

    private static void servePhoto(String name, OutputStream out) throws IOException {
        JSONObject contacts = loadData();
    
        if (!contacts.has(name) || !contacts.getJSONObject(name).has("photo") || 
            contacts.getJSONObject(name).getString("photo").isEmpty()) {
            sendResponse(out, 404, "Not Found", "text/plain", "Photo not found".getBytes());
            return;
        }
        
        String photoBase64 = contacts.getJSONObject(name).getString("photo");
        byte[] photoData = Base64.getDecoder().decode(photoBase64);
        
        // Detect image type from magic bytes
        String imageType = "jpeg";
        if (photoData.length > 2) {
            if (photoData[0] == (byte)0x89 && photoData[1] == (byte)0x50 && photoData[2] == (byte)0x4E) {
                imageType = "png";
            } else if (photoData[0] == (byte)0x47 && photoData[1] == (byte)0x49 && photoData[2] == (byte)0x46) {
                imageType = "gif";
            }
        }
        
        // Create response with caching headers
        StringBuilder responseBuilder = new StringBuilder();
        responseBuilder.append("HTTP/1.1 200 OK\r\n");
        responseBuilder.append("Content-Type: image/").append(imageType).append("\r\n");
        responseBuilder.append("Content-Length: ").append(photoData.length).append("\r\n");
        responseBuilder.append("Cache-Control: max-age=3600\r\n");
        responseBuilder.append("ETag: \"").append(name.hashCode()).append("\"\r\n");
        responseBuilder.append("Connection: close\r\n");
        responseBuilder.append("\r\n");

        out.write(responseBuilder.toString().getBytes());
        out.write(photoData);
        out.flush();
    }

    private static void serveApiContacts(OutputStream out) throws IOException {
        JSONObject contacts = loadData();
        JSONObject result = new JSONObject();
        
        for (String name : contacts.keySet()) {
            JSONObject contact = contacts.getJSONObject(name);
            JSONObject simplified = new JSONObject();
            simplified.put("phone", contact.getString("phone"));
            simplified.put("has_photo", contact.has("photo") && !contact.getString("photo").isEmpty());
            result.put(name, simplified);
        }
        
        sendResponse(out, 200, "OK", "application/json", result.toString(2).getBytes());
    }

    private static void serveApiContact(String name, OutputStream out) throws IOException {
        JSONObject contacts = loadData();
        
        if (!contacts.has(name)) {
            JSONObject error = new JSONObject();
            error.put("error", "Contact not found");
            sendResponse(out, 404, "Not Found", "application/json", error.toString().getBytes());
            return;
        }
        
        JSONObject contact = contacts.getJSONObject(name);
        JSONObject result = new JSONObject();
        result.put("name", name);
        result.put("phone", contact.getString("phone"));
        result.put("has_photo", contact.has("photo") && !contact.getString("photo").isEmpty());
        
        sendResponse(out, 200, "OK", "application/json", result.toString(2).getBytes());
    }

    private static void serveErrorPage(OutputStream out, int statusCode, String statusText, String message) throws IOException {
        Map<String, Object> templateVars = new HashMap<>();
        templateVars.put("error", statusText);
        templateVars.put("message", message);
        
        String content = renderTemplate("templates/error.html", templateVars);
        sendResponse(out, statusCode, statusText, "text/html", content.getBytes());
    }

    // Contact handling methods
    private static void handleAddContact(Map<String, String> headers, String body, OutputStream out) throws IOException {
        if (!headers.getOrDefault("content-type", "").startsWith("multipart/form-data")) {
            serveErrorPage(out, 400, "Bad Request", "Invalid content type.");
            return;
        }
        
        String boundary = getBoundary(headers.get("content-type"));
        if (boundary == null) {
            serveErrorPage(out, 400, "Bad Request", "Invalid multipart form data.");
            return;
        }
        
        // Parse form data
        Map<String, String> formData = parseMultipartForm(body, boundary);
        String name = formData.get("name");
        String phone = formData.get("phone");
        String photoData = formData.get("photo");
        
        if (name == null || name.isEmpty() || phone == null || phone.isEmpty()) {
            Map<String, Object> templateVars = new HashMap<>();
            templateVars.put("error", "Name and phone are required");
            String content = renderTemplate("templates/add.html", templateVars);
            sendResponse(out, 400, "Bad Request", "text/html", content.getBytes());
            return;
        }
        
        JSONObject contacts = loadData();
        
        // Check if name already exists
        if (contacts.has(name)) {
            Map<String, Object> templateVars = new HashMap<>();
            templateVars.put("error", "Contact '" + name + "' already exists");
            String content = renderTemplate("templates/add.html", templateVars);
            sendResponse(out, 409, "Conflict", "text/html", content.getBytes());
            return;
        }
        
        // Create new contact
        JSONObject contact = new JSONObject();
        contact.put("phone", phone);
        if (photoData != null && !photoData.isEmpty()) {
            contact.put("photo", photoData);
        }
        
        contacts.put(name, contact);
        saveData(contacts);
        
        // Redirect to index page
        sendRedirect(out, "/");
    }

    private static void handleEditContact(String name, Map<String, String> headers, String body, OutputStream out) throws IOException {
        JSONObject contacts = loadData();
        
        if (!contacts.has(name)) {
            serveErrorPage(out, 404, "Not Found", "Contact not found.");
            return;
        }
        
        if (!headers.getOrDefault("content-type", "").startsWith("multipart/form-data")) {
            serveErrorPage(out, 400, "Bad Request", "Invalid content type.");
            return;
        }
        
        String boundary = getBoundary(headers.get("content-type"));
        if (boundary == null) {
            serveErrorPage(out, 400, "Bad Request", "Invalid multipart form data.");
            return;
        }
        
        // Parse form data
        Map<String, String> formData = parseMultipartForm(body, boundary);
        String newName = formData.get("name");
        String phone = formData.get("phone");
        String photoData = formData.get("photo");
        
        if (newName == null || newName.isEmpty() || phone == null || phone.isEmpty()) {
            Map<String, Object> templateVars = new HashMap<>();
            templateVars.put("name", name);
            templateVars.put("contact", contacts.getJSONObject(name));
            templateVars.put("error", "Name and phone are required");
            String content = renderTemplate("templates/edit.html", templateVars);
            sendResponse(out, 400, "Bad Request", "text/html", content.getBytes());
            return;
        }
        
        if (!newName.equals(name) && contacts.has(newName)) {
            Map<String, Object> templateVars = new HashMap<>();
            templateVars.put("name", name);
            templateVars.put("contact", contacts.getJSONObject(name));
            templateVars.put("error", "Contact '" + newName + "' already exists");
            String content = renderTemplate("templates/edit.html", templateVars);
            sendResponse(out, 409, "Conflict", "text/html", content.getBytes());
            return;
        }
        
        // Update the contact
        JSONObject contact = contacts.getJSONObject(name);
        contact.put("phone", phone);
        
        if (photoData != null && !photoData.isEmpty()) {
            contact.put("photo", photoData);
        }
        
        if (!newName.equals(name)) {
            contacts.remove(name);
        }
        
        contacts.put(newName, contact);
        saveData(contacts);
        
        // Redirect to index page
        sendRedirect(out, "/");
    }

    private static void handleDeleteContact(String name, OutputStream out) throws IOException {
        JSONObject contacts = loadData();
        
        if (!contacts.has(name)) {
            serveErrorPage(out, 404, "Not Found", "Contact not found.");
            return;
        }
        
        contacts.remove(name);
        saveData(contacts);
        
        // Redirect to index page
        sendRedirect(out, "/");
    }

    // Helper methods
    private static String getBoundary(String contentType) {
        if (contentType == null) return null;
        
        Pattern pattern = Pattern.compile("boundary=(.+)");
        Matcher matcher = pattern.matcher(contentType);
        if (matcher.find()) {
            return matcher.group(1);
        }
        return null;
    }

    private static Map<String, String> parseMultipartForm(String body, String boundary) {
        Map<String, String> formData = new HashMap<>();
        
        // Split by boundary
        String[] parts = body.split("--" + boundary);
        
        for (String part : parts) {
            if (part.trim().isEmpty() || part.trim().equals("--")) {
                continue;
            }
            
            // Split header and content
            int headerEndIndex = part.indexOf("\r\n\r\n");
            if (headerEndIndex == -1) continue;
            
            String header = part.substring(0, headerEndIndex);
            String content = part.substring(headerEndIndex + 4);
            
            // Remove trailing \r\n if present
            if (content.endsWith("\r\n")) {
                content = content.substring(0, content.length() - 2);
            }
            
            // Extract field name
            Pattern namePattern = Pattern.compile("name=\"([^\"]+)\"");
            Matcher nameMatcher = namePattern.matcher(header);
            if (!nameMatcher.find()) {
                continue;
            }
            
            String fieldName = nameMatcher.group(1);
            
            // Check if it's a file upload
            if (header.contains("filename=") && fieldName.equals("photo")) {
                // Extract content type
                Pattern contentTypePattern = Pattern.compile("Content-Type: ([^\r\n]+)");
                Matcher contentTypeMatcher = contentTypePattern.matcher(header);
                if (contentTypeMatcher.find()) {
                    String contentType = contentTypeMatcher.group(1).trim();
                    if (contentType.startsWith("image/")) {
                        byte[] binaryData = content.getBytes(StandardCharsets.ISO_8859_1);
                        
                        // Base64 encode the binary data
                        String base64Data = Base64.getEncoder().encodeToString(binaryData);
                        formData.put(fieldName, base64Data);
                    }
                }
            } else {
                // Normal form field
                formData.put(fieldName, content);
            }
        }
        
        return formData;
    }

    private static String renderTemplate(String templatePath, Map<String, Object> variables) {
        try {
            String template = new String(Files.readAllBytes(Paths.get(templatePath)));
            
            // Simple template rendering
            for (Map.Entry<String, Object> entry : variables.entrySet()) {
                String key = entry.getKey();
                Object value = entry.getValue();
                
                if (value instanceof JSONObject) {
                    JSONObject jsonObj = (JSONObject) value;
                    
                    // Handle contacts variable
                    if (key.equals("contacts")) {
                        StringBuilder contactsHtml = new StringBuilder();
                        
                        if (jsonObj.length() == 0) {
                            String emptyMessage = "<p class=\"empty-message\">No contacts yet. <a href=\"/add\">Add one!</a></p>";
                            template = template.replace("{% if contacts %}\n                    {% for name, contact in contacts.items() %}\n                        <div class=\"contact\">\n                            {% if contact|has_photo %}\n                                <img src=\"/photo/{{ name }}\" alt=\"{{ name }}\">\n                            {% endif %}\n                            <div class=\"contact-info\">\n                                <h3>{{ name }}</h3>\n                                <p>Phone: {{ contact.phone }}</p>\n                                <div class=\"actions\">\n                                    <a href=\"/edit/{{ name }}\">Edit</a>\n                                    <a href=\"/delete/{{ name }}\">Delete</a>\n                                </div>\n                            </div>\n                            <div style=\"clear: both;\"></div>\n                        </div>\n                    {% endfor %}\n                {% else %}\n                    " + emptyMessage + "\n                {% endif %}", emptyMessage);
                        } else {
                            for (String contactName : jsonObj.keySet()) {
                                JSONObject contact = jsonObj.getJSONObject(contactName);
                                
                                StringBuilder contactHtml = new StringBuilder();
                                contactHtml.append("<div class=\"contact\">\n");
                                
                                // Photo
                                if (contact.has("photo") && !contact.getString("photo").isEmpty()) {
                                    contactHtml.append("    <img src=\"/photo/").append(contactName).append("\" alt=\"").append(contactName).append("\">\n");
                                }
                                
                                // Contact info
                                contactHtml.append("    <div class=\"contact-info\">\n");
                                contactHtml.append("        <h3>").append(contactName).append("</h3>\n");
                                contactHtml.append("        <p>Phone: ").append(contact.getString("phone")).append("</p>\n");
                                contactHtml.append("        <div class=\"actions\">\n");
                                contactHtml.append("            <a href=\"/edit/").append(contactName).append("\">Edit</a>\n");
                                contactHtml.append("            <a href=\"/delete/").append(contactName).append("\">Delete</a>\n");
                                contactHtml.append("        </div>\n");
                                contactHtml.append("    </div>\n");
                                contactHtml.append("    <div style=\"clear: both;\"></div>\n");
                                contactHtml.append("</div>\n");
                                
                                contactsHtml.append(contactHtml);
                            }
                            
                            template = template.replace("{% if contacts %}\n                    {% for name, contact in contacts.items() %}\n                        <div class=\"contact\">\n                            {% if contact|has_photo %}\n                                <img src=\"/photo/{{ name }}\" alt=\"{{ name }}\">\n                            {% endif %}\n                            <div class=\"contact-info\">\n                                <h3>{{ name }}</h3>\n                                <p>Phone: {{ contact.phone }}</p>\n                                <div class=\"actions\">\n                                    <a href=\"/edit/{{ name }}\">Edit</a>\n                                    <a href=\"/delete/{{ name }}\">Delete</a>\n                                </div>\n                            </div>\n                            <div style=\"clear: both;\"></div>\n                        </div>\n                    {% endfor %}\n                {% else %}\n                    <p class=\"empty-message\">No contacts yet. <a href=\"/add\">Add one!</a></p>\n                {% endif %}", contactsHtml.toString());
                        }
                    }
                } else {
                    // Handle simple variable replacements
                    template = template.replace("{{ " + key + " }}", value == null ? "" : value.toString());
                    
                    // Handle conditional blocks
                    if (key.equals("query") && value != null && !value.toString().isEmpty()) {
                        template = template.replace("{% if query %}", "").replace("{% endif %}", "");
                    }
                    
                    // Handle error message
                    if (key.equals("error") && value != null) {
                        template = template.replace("{% if error %}\n                    <p class=\"error\">{{ error }}</p>\n                {% endif %}", "<p class=\"error\">" + value + "</p>");
                    } else {
                        template = template.replace("{% if error %}\n                    <p class=\"error\">{{ error }}</p>\n                {% endif %}", "");
                    }
                }
            }
            
            // Remove any remaining template tags
            template = template.replaceAll("\\{\\{[^}]*\\}\\}", "");
            template = template.replaceAll("\\{\\%[^%]*\\%\\}", "");
            
            return template;
        } catch (IOException e) {
            System.err.println("Error rendering template: " + e.getMessage());
            return "<html><body><h1>Error</h1><p>Failed to render template.</p></body></html>";
        }
    }

    private static void sendResponse(OutputStream out, int statusCode, String statusText, String contentType, byte[] content) throws IOException {
        StringBuilder responseBuilder = new StringBuilder();
        responseBuilder.append("HTTP/1.1 ").append(statusCode).append(" ").append(statusText).append("\r\n");
        responseBuilder.append("Content-Type: ").append(contentType).append("\r\n");
        responseBuilder.append("Content-Length: ").append(content.length).append("\r\n");
        responseBuilder.append("Connection: close\r\n");
        responseBuilder.append("\r\n");
        
        out.write(responseBuilder.toString().getBytes());
        out.write(content);
        out.flush();
    }

    private static void sendRedirect(OutputStream out, String location) throws IOException {
        StringBuilder responseBuilder = new StringBuilder();
        responseBuilder.append("HTTP/1.1 303 See Other\r\n");
        responseBuilder.append("Location: ").append(location).append("\r\n");
        responseBuilder.append("Content-Length: 0\r\n");
        responseBuilder.append("Connection: close\r\n");
        responseBuilder.append("\r\n");
        
        out.write(responseBuilder.toString().getBytes());
        out.flush();
    }

    private static String urlDecode(String value) {
        try {
            return URLDecoder.decode(value, "UTF-8");
        } catch (UnsupportedEncodingException e) {
            return value;
        }
    }


    private static void handleClient(Socket clientSocket) {
        try (
            BufferedReader in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
            OutputStream out = clientSocket.getOutputStream()
        ) {
            // Parse the HTTP request
            String requestLine = in.readLine();
            if (requestLine == null) {
                return;
            }

            String[] requestParts = requestLine.split(" ");
            if (requestParts.length != 3) {
                sendResponse(out, 400, "Bad Request", "text/plain", "Invalid HTTP request".getBytes());
                return;
            }

            String method = requestParts[0];
            String path = requestParts[1];
            
            // Read headers
            Map<String, String> headers = new HashMap<>();
            String headerLine;
            while ((headerLine = in.readLine()) != null && !headerLine.isEmpty()) {
                int colonPos = headerLine.indexOf(':');
                if (colonPos > 0) {
                    String headerName = headerLine.substring(0, colonPos).trim();
                    String headerValue = headerLine.substring(colonPos + 1).trim();
                    headers.put(headerName.toLowerCase(), headerValue);
                }
            }

            // Handle different request types
            if (method.equals("GET")) {
                handleGetRequest(path, headers, out);
            } else if (method.equals("POST")) {
                // Read content length
                int contentLength = 0;
                if (headers.containsKey("content-length")) {
                    try {
                        contentLength = Integer.parseInt(headers.get("content-length"));
                    } catch (NumberFormatException e) {
                        sendResponse(out, 400, "Bad Request", "text/plain", "Invalid Content-Length".getBytes());
                        return;
                    }
                }

                if (contentLength > MAX_CONTENT_LENGTH) {
                    sendResponse(out, 413, "Payload Too Large", "text/plain", "Content too large".getBytes());
                    return;
                }

                // Read request body for POST requests
                char[] buffer = new char[contentLength];
                in.read(buffer, 0, contentLength);
                String body = new String(buffer);

                handlePostRequest(path, headers, body, out);
            } else {
                sendResponse(out, 405, "Method Not Allowed", "text/plain", "Method not supported".getBytes());
            }
        } catch (IOException e) {
            System.err.println("Error handling client: " + e.getMessage());
        } finally {
            try {
                clientSocket.close();
            } catch (IOException e) {
                System.err.println("Error closing client socket: " + e.getMessage());
            }
        }
    }

    private static void handlePostRequest(String path, Map<String, String> headers, String body, OutputStream out) throws IOException {
        if (path.equals("/add")) {
            handleAddContact(headers, body, out);
        } else if (path.startsWith("/edit/")) {
            String name = path.substring("/edit/".length());
            handleEditContact(urlDecode(name), headers, body, out);
        } else if (path.startsWith("/delete/")) {
            String name = path.substring("/delete/".length());
            handleDeleteContact(urlDecode(name), out);
        } else {
            // Return 404 Not Found
            serveErrorPage(out, 404, "Not Found", "The requested resource does not exist.");
        }
    }

    // JSON data handling methods
    private static JSONObject loadData() {
        try {
            if (Files.exists(Paths.get(DATA_FILE))) {
                String content = new String(Files.readAllBytes(Paths.get(DATA_FILE)));
                return new JSONObject(content);
            }
        } catch (Exception e) {
            System.err.println("Error loading data: " + e.getMessage());
        }
        return new JSONObject();
    }

    private static void saveData(JSONObject data) {
        try {
            Files.write(Paths.get(DATA_FILE), data.toString(2).getBytes());
        } catch (Exception e) {
            System.err.println("Error saving data: " + e.getMessage());
        }
    }
}

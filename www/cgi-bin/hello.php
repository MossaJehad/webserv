<?php
// Third CGI language, dispatched by the .php extension via php-cgi.
header("Content-Type: text/html");

$method = isset($_SERVER["REQUEST_METHOD"]) ? $_SERVER["REQUEST_METHOD"] : "";
$query  = isset($_SERVER["QUERY_STRING"]) ? $_SERVER["QUERY_STRING"] : "";
$body   = file_get_contents("php://input");

echo "<!DOCTYPE html>\n";
echo "<html><head><title>PHP CGI</title></head>\n";
echo "<body style=\"font-family: Arial, sans-serif;\">\n";
echo "<h1>Hello from PHP CGI</h1>\n";
echo "<p>REQUEST_METHOD: " . htmlspecialchars($method) . "</p>\n";
echo "<p>QUERY_STRING: " . htmlspecialchars($query) . "</p>\n";
echo "<p>Body length: " . strlen($body) . "</p>\n";
echo "<p>Working directory: " . htmlspecialchars(getcwd()) . "</p>\n";
echo "</body></html>\n";

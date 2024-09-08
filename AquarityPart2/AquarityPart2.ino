#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

const char* ssid = "Galaxy A54 5G 7E3B";
const char* password = "CzechRepublic";

const char* adminUsername = "admin";
const char* adminPassword = "admin123";

ESP8266WebServer server(80);
Servo servoMotor;

#define TURBIDITY_PIN A0
#define SERVO_PIN 14 // Define the pin for the servo motor

bool isAuthenticated = false;
int calibratedValue = 0;
bool servoActive = false;  // Track the state of the servo

int sensorToNtu(int sensor) {
  int volt = (sensor * (3.3/1024) * 1000);
  return 0.000704*volt*volt - 4.61*volt + 7535;
}

int calibrate() {
  int totalValue = 0;
  int n = 6;
  for (int i = 0; i < n; i++) {
    totalValue += analogRead(TURBIDITY_PIN);
    Serial.println(totalValue);
    delay(1000);
  }
  int avg = totalValue/n;
  return sensorToNtu(avg) - (0.1408*sensorToNtu(avg) - 461);
}

int readTurbidity() {
  int sensorValue = analogRead(TURBIDITY_PIN);
  return sensorToNtu(sensorValue);
}

void handleLogin() {
  if (server.method() == HTTP_POST) {
    if (server.arg("username") == adminUsername && server.arg("password") == adminPassword) {
      isAuthenticated = true;
      server.sendHeader("Location", "/calibrating");
      server.send(302, "text/plain", "");
    } else {
      server.send(200, "text/html", "<html><body><h1>Login Failed</h1><a href='/login'>Try again</a></body></html>");
    }
  } else {
    String html = "<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html += "<style>@import url('https://fonts.googleapis.com/css2?family=Roboto:wght@700&display=swap');";
    html += "body { background: url('https://i.postimg.cc/wM5LQ7jt/water.jpg') no-repeat center center fixed; background-size: cover; font-family: Arial, sans-serif; color: #fff; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }";
    html += ".login-container { background: rgba(255, 255, 255, 0.8); padding: 30px; width: 90%; max-width: 400px; border-radius: 10px; box-shadow: 0 0 15px rgba(0, 0, 0, 0.4); transform: skew(-5deg); position: relative; z-index: 2; }";
    html += ".login-container::before { content: ''; position: absolute; top: -10px; left: -10px; right: -10px; bottom: -10px; background: rgba(0, 123, 255, 0.1); z-index: -1; border-radius: 15px; transform: skew(5deg); box-shadow: 0 0 20px rgba(0, 0, 0, 0.2); }";
    html += "input { margin-bottom: 15px; padding: 15px; width: 100%; border-radius: 5px; border: 1px solid #ddd; box-shadow: inset 0 0 5px rgba(0, 0, 0, 0.1); transform: skew(5deg); }";
    html += "input:focus { border-color: #007bff; box-shadow: 0 0 5px rgba(0, 123, 255, 0.5); outline: none; }";
    html += "input[type='submit'] { background-color: #007bff; color: white; cursor: pointer; border: none; border-radius: 5px; padding: 15px; transform: skew(5deg); }";
    html += "input[type='submit']:hover { background-color: #0056b3; }";
    html += "h1 { font-family: 'Roboto', sans-serif; color: #007bff; text-align: center; margin-bottom: 20px; transform: skew(5deg); }</style></head>";
    html += "<body><div class=\"login-container\"><h1>AQUARITY</h1><form method='POST'><input type='text' name='username' placeholder='Username' required>";
    html += "<input type='password' name='password' placeholder='Password' required><input type='submit' value='Login'></form></div></body></html>";

    server.send(200, "text/html", html);

  }
}


void handleRoot() {
  if (!isAuthenticated) {
    server.sendHeader("Location", "/login");
    server.send(302, "text/plain", "");
    return;
  }

  String buttonText = servoActive ? "Deactivate Servo" : "Activate Servo";

  String html = "<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>@import url('https://fonts.googleapis.com/css2?family=Lobster&display=swap');";
  html += "body { background: url('https://i.postimg.cc/sftZvPKT/wateraerial.jpg') no-repeat center center fixed; background-size: cover; font-family: Arial, sans-serif; color: #fff; text-align: center; display: flex; flex-direction: column; align-items: center; justify-content: space-around; height: 100vh; margin: 0; }";
  html += ".container { width: 80%; max-width: 500px; background-color: rgba(0, 51, 102, 0.8); padding: 20px; border-radius: 15px; box-shadow: 0 10px 20px rgba(0, 0, 0, 0.2); transform: rotate(-3deg); }";
  html += ".container h2 { font-family: 'Lobster', cursive; font-size: 36px; color: #00bfff; margin-bottom: 10px; }";
  html += "#turbidityDisplay { font-size: 50px; font-weight: bold; margin: 20px 0; color: #d6ebf2; }";
  html += "#notificationBox { background-color: #ff4500; color: white; padding: 10px; border-radius: 15px; display: none; box-shadow: 0 5px 15px rgba(0, 0, 0, 0.3); margin-bottom: 20px; }";
  html += "#servoBox { padding: 20px; border-radius: 10px; margin-top: 20px; background-color: #fefeff; box-shadow: 0 10px 20px rgba(0, 0, 0, 0.3); transform: rotate(3deg); }";
  html += "#servoButton { background-color: #1e90ff; color: white; padding: 15px 30px; font-size: 18px; border: none; border-radius: 50px; cursor: pointer; box-shadow: 0 5px 15px rgba(0, 0, 0, 0.2); transition: all 0.3s ease; }";
  html += "#servoButton:hover { background-color: #104e8b; transform: scale(1.05); }";
  html += "footer { width: 100%; padding: 10px; background-color: #34495e; color: white; position: absolute; bottom: 0; }";
  html += "#logoutBox a { color: #1b7593; text-decoration: none; font-size: 16px; }";
  html += "#logoutBox a:hover { text-decoration: underline; }";
  html += ".footer-text { font-size: 14px; color: #aaa; margin-top: 5px; }";
  html += "</style></head>";
  html += "<body><div class=\"container\"><h2>AQUARITY</h2>";
  html += "<div id=\"turbidityDisplay\">-- mNTU / " + String(calibratedValue) + " mNTU</div>";
  html += "<div id=\"notificationBox\"><h3>Warning - Water is impure! " + String(calibratedValue) + " NTU</h3></div></div>";
  html += "<div id=\"servoBox\"><button id=\"servoButton\" onclick=\"toggleServo()\">" + buttonText + "</button></div>";
  html += "<footer><div id=\"logoutBox\"><a href='/logout'>Logout</a></div><div class=\"footer-text\">&copy; 2024 Digital Aquarity Systems</div></footer>";
  html += "<script>let servoActive = " + String(servoActive ? "true" : "false") + "; function updateData() { fetch('/turbidity').then(response => response.json()).then(data => { document.getElementById('turbidityDisplay').innerText = data.turbidity + ' mNTU / " + String(calibratedValue) + " mNTU'; if (data.turbidity > " + String(calibratedValue) + ") { document.getElementById('notificationBox').style.display = 'block'; } else { document.getElementById('notificationBox').style.display = 'none'; } }); }";
  html += "function toggleServo() { servoActive = !servoActive; const button = document.getElementById('servoButton'); button.innerText = servoActive ? 'Deactivate Motor' : 'Activate Motor'; fetch('/toggleServo?state=' + (servoActive ? 'on' : 'off')); }";
  html += "setInterval(updateData, 1000);</script></body></html>";

  server.send(200, "text/html", html);
}


void handleCalibrating() {
  String html = "<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>@import url('https://fonts.googleapis.com/css2?family=Lobster&display=swap');";
  html += "body { font-family: Arial, sans-serif; background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%), url('/water_pattern.png'); color: white; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }";
  html += ".calibrating-container { background: rgba(255, 255, 255, 0.1); padding: 20px; text-align: center; border-radius: 10px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.2); }";
  html += "h1 { font-family: 'Lobster', cursive; }</style></head>";
  html += "<body><div class=\"calibrating-container\"><h1>AQUARITY</h1><p>Calibrating...</p></div>";
  html += "<script>setTimeout(function() { window.location.href = '/calibrate'; }, 2000);</script></body></html>";

  server.send(200, "text/html", html);
}


void handleToggleServo() {
  String state = server.arg("state");
  if (state == "on") {
    servoMotor.write(180);  // Activate the servo
    servoActive = true;
  } else if (state == "off") {
    servoMotor.write(0);  // Deactivate the servo
    servoActive = false;
  }
  server.send(200, "text/plain", "Servo toggled");
}

void handleLogout() {
  isAuthenticated = false;
  server.sendHeader("Location", "/login");
  server.send(302, "text/plain", "");
}

void handleCalibrate() {
  calibratedValue = calibrate();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleTurbidity() {
  if (!isAuthenticated) {
    server.sendHeader("Location", "/login");
    server.send(302, "text/plain", "");
    return;
  }

  int turbidity = readTurbidity();
  String json = "{\"turbidity\": " + String(turbidity) + "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  pinMode(TURBIDITY_PIN, INPUT);
  servoMotor.attach(SERVO_PIN);  // Attach the servo to the defined pin

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  server.on("/login", handleLogin);
  server.on("/", handleRoot);
  server.on("/turbidity", handleTurbidity);
  server.on("/calibrating", handleCalibrating);
  server.on("/calibrate", handleCalibrate);
  server.on("/toggleServo", handleToggleServo);
  server.on("/logout", handleLogout);
  server.begin();
}

void loop() {
  server.handleClient();
}


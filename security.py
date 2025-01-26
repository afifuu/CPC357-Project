import os
import ssl
import paho.mqtt.client as mqtt
from google.cloud import secretmanager

# Fetch secrets securely from Google Cloud Secret Manager
def get_secret(secret_name):
    client = secretmanager.SecretManagerServiceClient()
    project_id = os.getenv("PROJECT_ID")
    name = f"projects/{project_id}/secrets/{secret_name}/versions/latest"
    response = client.access_secret_version(request={"name": name})
    return response.payload.data.decode("UTF-8")

# Load sensitive data securely
WIFI_SSID = get_secret("wifi_ssid")
WIFI_PASSWORD = get_secret("wifi_password")
MQTT_BROKER = get_secret("mqtt_broker")
MQTT_PORT = int(get_secret("mqtt_port"))
MQTT_USERNAME = get_secret("mqtt_username")
MQTT_PASSWORD = get_secret("mqtt_password")
CA_CERT_PATH = get_secret("ca_cert_path")
CLIENT_CERT_PATH = get_secret("client_cert_path")
CLIENT_KEY_PATH = get_secret("client_key_path")

# Configure MQTT client with secure settings
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker!")
    else:
        print(f"Failed to connect, return code {rc}")

def on_message(client, userdata, msg):
    print(f"Received message: {msg.payload.decode()} on topic {msg.topic}")

client = mqtt.Client()
client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
client.tls_set(
    ca_certs=CA_CERT_PATH,
    certfile=CLIENT_CERT_PATH,
    keyfile=CLIENT_KEY_PATH,
    tls_version=ssl.PROTOCOL_TLSv1_2
)
client.on_connect = on_connect
client.on_message = on_message

try:
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_start()
    client.subscribe("test/topic")
    client.publish("test/topic", "Hello, Secure World!")
except Exception as e:
    print(f"Connection failed: {e}")
finally:
    client.loop_stop()
    client.disconnect()

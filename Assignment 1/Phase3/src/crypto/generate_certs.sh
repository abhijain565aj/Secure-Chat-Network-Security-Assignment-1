#!/usr/bin/env bash

set -euo pipefail

if [ $# -eq 1 ]; then
    BUILD_DIR=$1
else
    echo "Usage: $0 <certs_directory>"
    exit 1
fi 

CERT_DIR=$BUILD_DIR/certs
mkdir -p $CERT_DIR

cd $CERT_DIR

echo "[1/7] Generating CA private key..."
openssl genrsa \
    -out ca.key \
    4096

echo "[2/7] Generating self-signed CA certificate..."
openssl req \
    -x509 \
    -new \
    -key ca.key \
    -sha256 \
    -days 3650 \
    -out ca.crt \
    -subj "/C=IN/ST=Maharashtra/L=Mumbai/O=MyChat/OU=CA/CN=MyChat Root CA"

echo "[3/7] Generating server private key..."
openssl genrsa \
    -out server.key \
    2048

echo "[4/7] Creating server certificate extensions..."
cat > server-ext.cnf <<'EOF'
subjectAltName = DNS:chat-server
extendedKeyUsage = serverAuth
EOF


echo "[5/7] Generating server certificate signing request..."
openssl req \
    -new \
    -key server.key \
    -out server.csr \
    -subj "/C=IN/ST=Maharashtra/L=Mumbai/O=MyChat/OU=Server/CN=chat-server"

echo "[6/7] Signing server certificate with CA..."
openssl x509 \
    -req \
    -in server.csr \
    -CA ca.crt \
    -CAkey ca.key \
    -CAcreateserial \
    -out server.crt \
    -days 365 \
    -sha256 \
    -extfile server-ext.cnf

echo "[7/7] Verifying server certificate..."
openssl verify \
    -CAfile ca.crt \
    server.crt

echo "========================================"
echo "       Certificate generation done"
echo "========================================"
echo
echo "Generated files:"
echo
echo "  ca.key        CA private key"
echo "  ca.crt        CA certificate"
echo "  server.key    Server private key"
echo "  server.csr    Server certificate request"
echo "  server.crt    CA-signed server certificate"
echo
echo "Certificate verification:"
openssl verify -CAfile ca.crt server.crt
echo
echo "========================================"
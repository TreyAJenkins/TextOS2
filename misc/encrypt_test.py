#!/usr/bin/env python3
#
# This is a simple script to encrypt a message using AES
# with CBC mode in Python 3.
# Before running it, you must install pycryptodome:
#
# $ python -m pip install PyCryptodome
#
# Author.: José Lopes
# Date...: 2019-06-14
# License: MIT
##


from hashlib import sha256
from base64 import b64decode
from base64 import b64encode

import struct

from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes
from Crypto.Util.Padding import pad, unpad

structstr = 'l'

class AESCipher:
    def __init__(self, key):
        self.key = sha256(key.encode('utf8')).digest()

    def encrypt(self, data):
        iv = get_random_bytes(AES.block_size)
        self.cipher = AES.new(self.key, AES.MODE_CBC, iv)
        return b64encode(iv + self.cipher.encrypt(pad(data.encode('utf-8'), 
            AES.block_size)))

    def decrypt(self, data):
        raw = b64decode(data)
        self.cipher = AES.new(self.key, AES.MODE_CBC, raw[:AES.block_size])
        return unpad(self.cipher.decrypt(raw[AES.block_size:]), AES.block_size)


#print('TESTING ENCRYPTION')
#msg = input('Message...: ')
#pwd = input('Password..: ')
#print('Ciphertext:', AESCipher(pwd).encrypt(msg).decode('utf-8'))

#print('\nTESTING DECRYPTION')
#cte = input('Ciphertext: ')
#pwd = input('Password..: ')
#print('Message...:', AESCipher(pwd).decrypt(cte).decode('utf-8'))

pwd = "TextOS"
msg = "Hello to the wonderful world of TextOS! This is a test of AES!! Can we fit multiple blocks? The answer is yes! Is there a limit? I don't know! The best way to find out would be to just try really big files!"
enc = AESCipher(pwd).encrypt(msg).decode('utf-8')
print(enc)

raw = b64decode(enc)
ln = struct.pack(structstr, len(raw))
raw = ln + raw
print("uint8_t raw[" + str(len(raw)) + "] = { ", end="")
for r in raw:
    print("0x%0.2X, " % r, end="")
print("};")
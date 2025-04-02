1) Description:

This project is a cryptographic digital will management system in which the the main idea is to encrypt a digital will document such that it remains confidential until and unless the will maker passes. This application assures safe storage and automatic execution using AES and RSA encryption techniques. 

2) Features:

Login and Registration for both the testator and beneficiary to ensure authenticity 
Beneficiary management 
Secure Will encryption and decryption using AES and RSA mechanisms
Hospital confirmation mechanism to verify the user's status 
Folder based user-data storage to store personal information such as aadhar, public key, registered beneficiaries, etc.


3) Technologies Used:

Programming Language: C++
Encryption Algorithms: AES, RSA
Storage: Local file system for encrypted wills and user data



4) Installation:

Clone the Repository:

git clone https://github.com/ctrler-aminx/Digital-Will-Management-using-Cryptography.git
cd Digital-Will-Management-using-Cryptography

Install Dependencies

Ensure you have a C++ compiler (G++)
Install OpenSSL for encryption support

Compile the Code 



5) Usage:

Hospital Confirmation:
Hospital gives confirmation that the testator is deceased

User and Beneficiary Registration/Login
Users and beneficiaries must register and authenticate themselves before accessing the system
Registration includes storing credentials securely
User login is only possible if he is alive

Creating a Will:
Entering the number of beneficiaries, their name and aadhar, number of properties and assign properties to the beneficiaries
Include additional text matter in the will
Encrypt and save the will

Will decryption:
Beneficiaries use their own private key to decrypt and view the will contents



6) Security Considerations:

End-to-End Encryption: ensures confidentiality of the will
Tamper Detection: Detects unauthorized access attempts

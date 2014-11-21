These certificates are for *TESTING ONLY*. It would be a very
STUPID IDEA TO CHECK IN YOUR *ACTUAL* CERTIFICATES!
The password these files were saved with is “eleven”.

They were created according to the directions at:

https://developer.apple.com/library/mac/technotes/tn2326/_index.html

Note that section “Exporting a Digital Identity” subsection “B. Select the Digital Identity” is,
at the time of this writing, misleading. The step-by-step instructions correctly tell you to
choose “My Certificates”, however the Note below it says “All Items” was equivalent. This is not true.
If you choose “All Items” you will not be able to select the “.p12” certificate format.

Also note that the .p12 certificates should be converted using the following command line:

    openssl pkcs12 -in ElevenServerCertificate.p12 -out ElevenServerCertificate.pem
    openssl x509 -inform der -in ElevenServerPublicCertificate.cer -out ElevenServerPublicCertificate.pem

Deployment
----------

You will need an actual TLS certificate from a trusted certificate authority (i.e. your web hoster), as a PEM file,
if you want to run this server in production. Create a local folder, and put

	settings.ini
	accounts.txt
	ElevenServerCertificate.pem

into that folder (ElevenServerCertificate.pem being the certificate you bought), then pass that folder's path to the server when you launch it. Create a
second folder for the client, containing

	settings.ini
	ElevenServerPublicCertificate.pem

and pass that folder's path to the client on launch. ElevenServerPublicCertificate.pem contains only the *public* certificate of the certificate you bought. This is important, as this certificate will be distributed to your users to allow the client to verify it is talking to your server. 

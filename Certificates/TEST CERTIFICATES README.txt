These certificates are for *TESTING ONLY*. The password these files were saved with is “eleven”.

They were created according to the directions at:

https://developer.apple.com/library/mac/technotes/tn2326/_index.html

Note that section “Exporting a Digital Identity” subsection “B. Select the Digital Identity” is, at the time of this writing, misleading. The step-by-step instructions correctly tell you to choose “My Certificates”, however the Note below it says “All Items” was equivalent. This is not true. If you choose “All Items” you will not be able to select the “.p12” certificate format.

Also note that the .p12 certificates should be exported using the following command line:

    openssl pkcs12 -in ElevenServerCertificate.p12 -out ElevenServerCertificate.pem

Deployment
----------

You will need an actual TLS certificate from a trusted certificate authority (i.e. your web hoster) if you want to run this server in production.
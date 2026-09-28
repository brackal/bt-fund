# Bluetooth Low Energy (BLE) – Cybersicherheit: Tutorials, Artikel, Bücher & Tools

Zusammenstellung von Ressourcen zum Thema Sicherheit bei Bluetooth Low Energy (BLE).

---

## Einstieg & Grundlagen

- **Wikipedia – Bluetooth Low Energy**
  Guter Überblick zu Technik und Einsatzgebieten.
  https://en.wikipedia.org/wiki/Bluetooth_Low_Energy

- **Analog Devices – Videoserie „All About BLE Security" (Teil 6 von 7)**
  Erklärt Pairing und Verschlüsselung verständlich.
  https://www.analog.com/en/resources/media-center/videos/6313217215112.html

- **Argenox – BLE Security & Privacy: A 2025 Guide**
  Pairing-Methoden, LE Secure Connections, MAC-Adress-Randomisierung.
  https://argenox.com/blog/bluetooth-low-energy-ble-security-privacy-a-2025-guide

- **Medium – BLE Security in a Nutshell (Panagiotis Antoniou)**
  Kompakte Einführung in Authentifizierung, Autorisierung und Schlüsseltypen (TK, LTK, CSRK).
  https://antonioupan6.medium.com/bluetooth-low-energy-ble-security-in-a-nutshell-5d20164cceb2

- **Cardinal Peak – How to Implement BLE Security and Privacy in Wireless Devices**
  https://www.cardinalpeak.com/blog/bluetooth-le-security-and-privacy-in-wireless-audio-devices

- **einfochips – BLE Security and Privacy for IoT**
  https://www.einfochips.com/blog/bluetooth-low-energy-ble-security-and-privacy-for-iot/

---

## Wissenschaftliche Artikel / Paper

- **„Securing Bluetooth Low Energy: A Literature Review"** (arXiv)
  Umfassender Überblick über Angriffe und Forschungsstand, inkl. Referenzen zu KNOB, BIAS, BLESA u.a.
  https://arxiv.org/pdf/2404.16846

- **„On the (In)security of Bluetooth Low Energy One-Way Secure Connections Only Mode"** (arXiv)
  https://arxiv.org/pdf/1908.10497

- **NCBI – Overview and Evaluation of Bluetooth Low Energy: An Emerging Low-Power Wireless Technology**
  https://www.ncbi.nlm.nih.gov/pmc/articles/PMC3478807/

---

## Praktische Tutorials / Exploitation

- **Attify Blog – „The Practical Guide to Hacking Bluetooth Low Energy"**
  Setup mit BlueZ, gatttool, nRF Connect App.
  https://blog.attify.com/the-practical-guide-to-hacking-bluetooth-low-energy/

- **Attify Blog – „Exploiting Bluetooth Low Energy using Gattacker for IoT"**
  Schritt-für-Schritt-Anleitung für einen Replay-Angriff auf eine Smart-Bulb.
  https://blog.attify.com/hacking-bluetooth-low-energy/

---

## Trainings- und Kurs-Curricula (gute Roadmaps zum Selbststudium)

- **HITB SecTrain – „Hands-On Hacking for BLE"** (Amsterdam 2022)
  Themen: Sniffing (nRF Sniffer, Ubertooth, Btlejack, Sniffle, SDR), HCI-Dumps, MITM (GATTacker, BtleJuice, Mirage), Web Bluetooth, Firmware-OTA-Angriffe.
  https://sectrain.hitb.org/courses/hands-on-hacking-for-ble-ams2022/

- **HITB SecConf 2023 Amsterdam – Hands-On Hacking for BLE**
  https://conference.hitb.org/hitbsecconf2023ams/product/hands-on-hacking-for-ble-hitb2023ams/

- **HITB (in)Cyber 2024 Abu Dhabi – Hands-On Hacking for BLE**
  https://conference.hitb.org/hitbincyber2024/product/hands-on-hacking-for-ble-hitb2023hkt/

- **HITB SecConf 2022 Singapore – In-Person Hands-On Hacking for BLE**
  Zusätzlich: BLE-Jamming/Hijacking, Firmware-OTA-Sicherheit.
  https://archive.conference.hitb.org/hitbsecconf2022sin/product/in-person-hands-on-hacking-for-ble-hitb2022sin/

- **HITB SecTrain – Virtual Hands-On Hacking for BLE**
  https://archive.conference.hitb.org/hitbsectrain2022ams/product/hands-on-hacking-for-ble-ams2022/

- **HITB SecTrain – A Practical Introduction To BLE Security (März 2021)**
  Fokus u.a. auf Btlejack (Jamming/Hijacking) und Web Bluetooth.
  https://sectrain.hitb.org/courses/a-practical-introduction-to-ble-security/

- **Lab401 Workshop – Hacking and Securing BLE and RFID devices**
  Dreitägiges Training zu Sniffing, Dumping, Spoofing, MITM, Jamming, Replay, Relay.
  https://lab401.com/pages/lab401-workshop-hacking-and-securing-ble-and-rfid-devices

---

## Bücher

- **„Practical IoT Hacking"** (No Starch Press, mehrere Autoren)
  Kapitel 11: Bluetooth Low Energy.
  https://nostarch.com/practical-iot-hacking
  Rezension: https://medium.com/@windsorheightsbookfair/practical-iot-hacking-book-review-b14b9f72c619

- **Aditya Gupta – „The IoT Hacker's Handbook: A Practical Guide to Hacking the Internet of Things"**
  Kapitel zu BLE/ZigBee-Exploitation (Sniffen, Erfassen, Ausnutzen von Funkprotokollen).
  - O'Reilly: https://www.oreilly.com/library/view/the-iot-hackers/9781484243008/
  - Springer: https://link.springer.com/book/10.1007/978-1-4842-4300-8
  - Goodreads: https://www.goodreads.com/book/show/42589222-the-iot-hacker-s-handbook

---

## Bekannte BLE-Schwachstellen/-Angriffe (Stichworte für eigene Recherche)

KNOB, BIAS, BLESA, BLURtooth, Sweyntooth, Bleedingbit, BlueFrag, Frankenstein, InjectaBLE, JackBNimBLE

*(Diese Begriffe tauchen in den oben verlinkten Papern und Kursbeschreibungen als Referenzen auf und eignen sich als Ausgangspunkt für vertiefte Recherche.)*

---

## Gängige frei verfügbare Tools

- **nRF Connect App** (Nordic Semiconductor) – Sniffing/Interaktion mit BLE-Geräten
- **BlueZ** (Linux Bluetooth-Stack) mit `gatttool`, `hcidump`
- **Wireshark** – Paketanalyse
- **Btlejack** – BLE-Sniffing, Jamming, Connection Hijacking
- **Ubertooth** – Hardware-Sniffer
- **Sniffle** – Open-Source-Sniffer
- **GATTacker / BtleJuice / Mirage** – MITM-/Relay-Angriffe

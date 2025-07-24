# -ESP32-Smart-Security-System-using-SIM800L
This project implements a smart security system using an ESP32, designed to detect intrusions, fire, and laser breaches, while providing real-time alerts via SMS and phone calls using the SIM800L GSM module.
![WhatsApp Image 2025-07-10 at 02 35 14_2c9ad875](https://github.com/user-attachments/assets/748a3afb-17f7-4b31-b2e7-9860ce316f5a)

## 🔧 Hardware Components

| Component           | Quantity | Description                                  |
|---------------------|----------|----------------------------------------------|
| ESP32 Dev Board     | 1        | Core microcontroller                         |
| SIM800L GSM Module  | 1        | For SMS and call alert functionality         |
| IR Sensor           | 2        | For entry/exit people counting               |
| Flame Sensor        | 1        | Detects fire/flame                           |
| LDR (Light Sensor)  | 1        | Used with laser to detect beam cut           |
| Laser Module        | 1        | For secure zone monitoring                   |
| Buzzer              | 1        | For audible alerts                           |
| I2C LCD (16x2)      | 1        | Displays system messages and status          |
| 12V Battery / Power | 1        | Stable power supply for SIM800L              |
| Resistors, wires, etc. | -    | Misc. components for setup                   |

---

## 🛠️ Features

- 🚶 **People Counting** using IR sensors
- 🔥 **Fire Detection** using flame sensor
- 🔦 **Laser Beam Cut Detection** using LDR
- 🔔 **Buzzer Alert** during emergencies
- 🧾 **SMS Alert** with current people count
- 📞 **Automated Call** to predefined number
- 📟 **LCD Display** with dynamic messages
- 📲 **SMS Command**: Send `Secure` to reset alerts

---

## 💡 How It Works

1. **IR Sensor Logic**  
   - IR1 → IR2: Person entered → increment count  
   - IR2 → IR1: Person exited → decrement count

2. **Trigger Conditions**  
   - **Fire Detected** (flame sensor = HIGH)  
   - **Laser Breach** (LDR reading drops below threshold)

3. **Actions on Trigger**
   - Buzzer turns ON  
   - Alert is displayed on LCD  
   - SMS with people count sent to user  
   - Phone call initiated using SIM800L  
   - System waits for user to send SMS `"Secure"` to silence buzzer

---

## 🧪 Sensor Pin Mapping

| Component         | ESP32 Pin |
|------------------|-----------|
| IR1              | GPIO15    |
| IR2              | GPIO4     |
| Flame Sensor     | GPIO5     |
| LDR Sensor       | GPIO32    |
| Buzzer           | GPIO2     |
| SIM800 RX        | GPIO26    |
| SIM800 TX        | GPIO27    |
| SIM800 RST       | GPIO25    |
| LCD SDA/SCL      | Default I2C (GPIO21, GPIO22) |

## Full Component Connection Schematic
![Smart_Security_Schematic](https://github.com/user-attachments/assets/bed00e7e-7b96-4738-9f27-8f40a670e5e1)


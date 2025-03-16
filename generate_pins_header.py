import re
import os

# Verzeichnis des Skripts ermitteln und Pfade für sdkconfig und Header-Datei setzen
script_dir = os.path.dirname(os.path.abspath(__file__))
sdkconfig_path = os.path.join(script_dir, "sdkconfig")
header_path = os.path.join(script_dir, "parsed_pins.h")

# Überprüfen, ob sdkconfig existiert
if not os.path.exists(sdkconfig_path):
    raise FileNotFoundError(f"sdkconfig file not found at {sdkconfig_path}")

# Definition der Konfigurationsvariablen, die jeweils 3 Pins enthalten sollen
config_entries = [
    ("CONFIG_I_SENSE_U_V_W_ADC", "CONFIG_I_SENSE_U_ADC", "CONFIG_I_SENSE_V_ADC", "CONFIG_I_SENSE_W_ADC"),
    ("CONFIG_HIN_U_V_W_GPIO", "CONFIG_HIN_U_GPIO", "CONFIG_HIN_V_GPIO", "CONFIG_HIN_W_GPIO"),
    ("CONFIG_LIN_U_V_W_GPIO", "CONFIG_LIN_U_GPIO", "CONFIG_LIN_V_GPIO", "CONFIG_LIN_W_GPIO"),
    ("CONFIG_HALL_A_B_C_GPIO", "CONFIG_HALL_A_GPIO", "CONFIG_HALL_B_GPIO", "CONFIG_HALL_C_GPIO"),
    ("CONFIG_IN_ENCODER_GPIO", "CONFIG_IN_ENC_A_GPIO", "CONFIG_IN_ENC_B_GPIO", "CONFIG_IN_ENC_BUT_GPIO"),
    ("CONFIG_EXT_ENCODER_GPIO", "CONFIG_EXT_ENC_INDX_GPIO","CONFIG_EXT_ENC_LEFT_GPIO", "CONFIG_EXT_ENC_RIGHT_GPIO"),
]

# Datei öffnen und auslesen
with open(sdkconfig_path, "r") as f:
    content = f.read()

# Header-Datei erzeugen
with open(header_path, "w") as header:
    header.write("// Automatically generated file. Do not modify.\n#ifndef PARSED_PINS_H\n#define PARSED_PINS_H\n\n")

    for config_var, pin1_name, pin2_name, pin3_name in config_entries:
        # Suche nach dem Konfigurationswert
        match = re.search(rf'{config_var}="([^"]+)"', content)
        if match:
            # Pins als Integer-Array extrahieren
            pins = match.group(1).split(",")
            
            # Prüfe, ob genau drei Pins angegeben sind
            if len(pins) != 3:
                raise ValueError(f"{config_var} muss genau drei Pins enthalten.")
            
            # Header-Einträge für die Pins hinzufügen
            header.write(f"#define {pin1_name} {pins[0].strip()}\n")
            header.write(f"#define {pin2_name} {pins[1].strip()}\n")
            header.write(f"#define {pin3_name} {pins[2].strip()}\n")
            header.write("\n")
            print(f"Parsed {config_var}: {pins[0].strip()}, {pins[1].strip()}, {pins[2].strip()}")
        else:
            print(f"Warning: {config_var} not found in sdkconfig")
    header.write("#endif")

print(f"Header file '{header_path}' generated successfully.")

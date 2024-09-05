import pandas as pd
import matplotlib.pyplot as plt

# Load the CSV file with headers
file_path = 'data.csv'  # Adjust the file path as needed
data = pd.read_csv(file_path)

# Extract DAC Value and Speed from the first row
dac_value = data['DAC Value'].iloc[0]
speed = data['Speed'].iloc[0]

# Print DAC and Speed values at the top
print(f"DAC Value: {dac_value}")
print(f"Speed: {speed}")

# Create plots
fig, axs = plt.subplots(2, 1, figsize=(10, 8), sharex=True)

# Add DAC and Speed values as separate text annotations
fig.suptitle('Data Analysis', fontsize=14, y=0.95)

# Add DAC and Speed values to the figure
fig.text(0.15, 0.97, f'DAC Value: {dac_value}', fontsize=12, ha='left')
fig.text(0.15, 0.93, f'Speed: {speed}', fontsize=12, ha='left')

# Plot Angle vs. Time
axs[0].plot(data['Total Time'], data['Angle'], marker='o', linestyle='-', color='b')
axs[0].set_title('Angle vs. Time')
axs[0].set_ylabel('Angle (degrees)')
axs[0].grid(True)

# Plot Direction vs. Time
axs[1].plot(data['Total Time'], data['Direction'], marker='o', linestyle='-', color='r')
axs[1].set_title('Direction vs. Time')
axs[1].set_xlabel('Time (seconds)')
axs[1].set_ylabel('Direction')
axs[1].grid(True)

# Adjust layout and show plots
plt.tight_layout(rect=[0, 0.05, 1, 0.95])  # Adjust the layout to fit the figure text
plt.show()


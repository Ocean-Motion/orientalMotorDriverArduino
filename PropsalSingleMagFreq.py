import matplotlib.pyplot as plt
import pandas as pd
import os
from matplotlib.ticker import MaxNLocator, FuncFormatter
import numpy as np

# Change directory to where the script and CSV file are located
os.chdir(r'C:\Users\jfunk\OneDrive\Documents\GitHub\omt-johnfunk\CSV_Pharing')

# Print the current working directory
print("Current working directory:", os.getcwd())

# Function to plot data from CSV
def plot_csv_data():
    # Read the CSV file and skip the header rows
    data = pd.read_csv('Data.csv', header=0)
    
    # Print first few rows of the data to debug
    print("First few rows of data:\n", data.head())
    
    # Check for any non-numeric values
    print("Data types:\n", data.dtypes)
    
    # Extract columns based on their order
    time = pd.to_numeric(data['Time'], errors='coerce')  # Convert to numeric, handle errors
    angular_speed = pd.to_numeric(data['AngularSpeed'], errors='coerce')  # Convert to numeric
    dac_value = pd.to_numeric(data['DACValue'], errors='coerce')  # Convert to numeric
    angle_pu = pd.to_numeric(data['AnglePU'], errors='coerce')  # Convert to numeric
    
    # Print min and max values to check for anomalies
    print("Angular Speed min:", angular_speed.min())
    print("Angular Speed max:", angular_speed.max())
    
    # Extract constant values from the last two rows (if applicable)
    magnitude_row = data.loc[data['Time'] == 'Magnitude']
    frequency_row = data.loc[data['Time'] == 'Frequency']
    
    if not magnitude_row.empty:
        magnitude = magnitude_row['AngularSpeed'].values[0]
    else:
        magnitude = 'Not Found'
    
    if not frequency_row.empty:
        frequency = frequency_row['AngularSpeed'].values[0]
    else:
        frequency = 'Not Found'
    
    return time, angular_speed, dac_value, angle_pu, magnitude, frequency

# Function to remove outliers
def remove_outliers(data_series):
    mean = np.mean(data_series)
    std_dev = np.std(data_series)
    lower_bound = mean - 2 * std_dev
    upper_bound = mean + 2 * std_dev
    return data_series[(data_series >= lower_bound) & (data_series <= upper_bound)]

# Custom formatter to reduce decimal places
def format_func(x, _):
    return f'{x:.2f}'

# Get data
time, angular_speed, dac_value, angle_pu, magnitude, frequency = plot_csv_data()

# Remove outliers
filtered_angular_speed = remove_outliers(angular_speed)
filtered_dac_value = remove_outliers(dac_value)
filtered_angle_pu = remove_outliers(angle_pu)

# Create subplots: 3 rows, 1 column with smaller size
fig, ax = plt.subplots(3, 1, figsize=(10, 12), sharex=True)

# Plot Angular Speed
ax[0].plot(time[~angular_speed.isna()], angular_speed.dropna(), label='Angular Speed', color='blue', linestyle='-', linewidth=1.0, marker='o', markersize=2)
ax[0].set_ylabel('Angular Speed', fontsize=12)
ax[0].legend(loc='upper right', fontsize=10)
ax[0].grid(True)

# Plot DAC Value
ax[1].plot(time[~dac_value.isna()], dac_value.dropna(), label='DAC Value', color='orange', linestyle='-', linewidth=1.0, marker='s', markersize=2)
ax[1].set_ylabel('DAC Value', fontsize=12)
ax[1].legend(loc='upper right', fontsize=10)
ax[1].grid(True)

# Plot Angle PU
ax[2].plot(time[~angle_pu.isna()], angle_pu.dropna(), label='Angle PU', color='green', linestyle='-', linewidth=1.0, marker='^', markersize=2)
ax[2].set_xlabel('Time', fontsize=12)
ax[2].set_ylabel('Angle PU', fontsize=12)
ax[2].legend(loc='upper right', fontsize=10)
ax[2].grid(True)

# Add space between the subplots
plt.subplots_adjust(hspace=0.3, top=0.85)  # Increase space between plots and adjust top

# Add text annotation with magnitude and frequency at the top of the figure
fig.text(0.5, 0.95, f'Magnitude: {magnitude}\nFrequency: {frequency}', ha='center', va='center', fontsize=12, weight='bold')

# Format x-axis ticks
for ax in fig.axes:
    ax.xaxis.set_major_locator(MaxNLocator(integer=True))  # Ensure x-axis has integer values
    ax.xaxis.set_major_formatter(FuncFormatter(format_func))  # Apply formatting

# Rotate x-axis labels for better readability
plt.xticks(rotation=45)

# Display the plots
plt.show()
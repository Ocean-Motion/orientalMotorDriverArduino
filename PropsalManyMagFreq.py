import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import os
from matplotlib.ticker import MaxNLocator, FuncFormatter

# Change directory to where the script and CSV file are located
os.chdir(r'C:\Users\jfunk\OneDrive\Documents\GitHub\omt-johnfunk\CSV_Pharing')

# Function to plot data from CSV
def plot_csv_data():
    # Read the CSV file
    data = pd.read_csv('Data.csv', header=0)
    return data

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
data = plot_csv_data()

# Convert 'Time' column to numeric to avoid errors
data['Time'] = pd.to_numeric(data['Time'], errors='coerce')

# Get unique intervals
intervals = data['Interval'].unique()

# Create subplots: 2 rows and 2 columns
fig, axs = plt.subplots(2, 2, figsize=(12, 8))

# Flatten the axes array for easier iteration
axs = axs.flatten()

# Define labels for each subplot
characteristics = ['DACValue', 'AngularSpeed', 'AnglePU']
labels = ['DAC Value', 'Angular Speed', 'Angle PU']

# Define a color palette excluding red
colors = ['blue', 'green', 'orange', 'purple', 'brown', 'pink', 'cyan', 'magenta']

# Generate the text for Magnitude and Frequency corresponding to each interval
mag_freq_text = ""
for i, interval in enumerate(intervals):
    if pd.isna(interval):
        continue  # Skip if the interval is NaN
    
    interval_data = data[data['Interval'] == interval]
    
    magnitude = interval_data['Magnitude'].iloc[0] if not interval_data['Magnitude'].isna().all() else None
    frequency = interval_data['Frequency'].iloc[0] if not interval_data['Frequency'].isna().all() else None
    
    mag_freq_text += f"Interval {int(interval)}: Magnitude = {magnitude:.2f}, Frequency = {frequency:.2f}\n"

# Add a title at the top of the figure
fig.suptitle('Analysis of Characteristics by Interval (Note Every Graph Is Over Time)', fontsize=16)

# Add a smaller text box for Magnitude and Frequency information above the graphs, with slightly raised position
plt.figtext(0.1, 0.93, f'{mag_freq_text.strip()}', fontsize=8, ha='left', va='top', bbox=dict(facecolor='white', alpha=0.6))

# Add the color legend slightly raised, next to the magnitude and frequency information
legend_text = '\n'.join([f'Interval {int(interval)}: {colors[i]}' for i, interval in enumerate(intervals) if not pd.isna(interval)])
plt.figtext(0.7, 0.93, legend_text, fontsize=8, ha='left', va='top', bbox=dict(facecolor='white', alpha=0.6))

# Adjust layout: Make space for the Angular Speed plot on the left
plt.subplots_adjust(left=0.08, right=0.95, top=0.85, bottom=0.1, hspace=0.3, wspace=0.3)

# Loop through each characteristic to generate a separate plot for each
for idx, (characteristic, label) in enumerate(zip(characteristics, labels)):
    ax = axs[idx]
    
    for i, interval in enumerate(intervals):
        if pd.isna(interval):
            continue  # Skip if the interval is NaN
        
        interval_data = data[data['Interval'] == interval]
        
        # Extract data for plotting
        time = interval_data['Time']  # Already numeric
        characteristic_data = pd.to_numeric(interval_data[characteristic], errors='coerce')  # Convert to numeric
        
        # Remove outliers
        filtered_data = remove_outliers(characteristic_data)
        filtered_time = time[characteristic_data.isin(filtered_data)]  # Corresponding time for filtered data
        
        # Plot filtered data for the current interval
        ax.plot(filtered_time, filtered_data, linestyle='-', linewidth=0.8, marker='o', markersize=3, color=colors[i % len(colors)])
    
    # Set labels and remove titles
    ax.set_ylabel(label, fontsize=9)
    
    # Add grid
    ax.grid(True)

    # Format x-axis ticks
    ax.xaxis.set_major_locator(MaxNLocator(integer=True))  # Ensure x-axis has integer values
    ax.xaxis.set_major_formatter(FuncFormatter(format_func))  # Apply formatting

    # Rotate x-axis labels for better readability
    ax.tick_params(axis='x', rotation=45)

# Remove the 4th subplot (Empty) by turning it off
axs[3].axis('off')

# Save the complete figure
plt.savefig('Characteristics_by_Interval_Smaller.png')

# Display the complete figure
plt.show()

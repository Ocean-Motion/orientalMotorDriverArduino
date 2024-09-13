import pandas as pd
import matplotlib.pyplot as plt
import os

# Function to reject outliers based on the standard deviation method
def reject_outliers(df, column, num_std_dev=2):
    mean = df[column].mean()  # Mean of the column
    std_dev = df[column].std()  # Standard deviation of the column
    lower_bound = mean - num_std_dev * std_dev  # Lower bound (mean - 2*std_dev)
    upper_bound = mean + num_std_dev * std_dev  # Upper bound (mean + 2*std_dev)
    return df[(df[column] >= lower_bound) & (df[column] <= upper_bound)]  # Filtered DataFrame

# File path to the CSV file generated by Arduino
file_path = 'C:/Users/jfunk/OneDrive/Documents/GitHub/omt-johnfunk/NmSec.csv'  # Use the correct path to your file

# Check if the file exists
if not os.path.exists(file_path):
    print(f"Error: The file '{file_path}' was not found. Please check the file path.")
else:
    # Read the CSV file into a DataFrame
    df = pd.read_csv(file_path)

    # Check the first few rows of the DataFrame to ensure it's read correctly
    print(df.head())

    # Convert time into seconds for the x-axis
    df['Time (s)'] = df['Time (m)'] * 60 + df['Time (s)']

    # Reject outliers from the 'Calibrated Torque (N·m)' column using the 2 standard deviations method
    df_filtered = reject_outliers(df, 'Calibrated Torque (N·m)', num_std_dev=2)

    # Plotting only calibrated torque after outlier rejection
    plt.figure(figsize=(10, 6))
    plt.plot(df_filtered['Time (s)'], df_filtered['Calibrated Torque (N·m)'], marker='x', linestyle='--', color='r', label="Calibrated Torque (Filtered)")
    plt.xlabel('Time (s)')
    plt.ylabel('Calibrated Torque (N·m)')
    plt.title('Calibrated Torque Over Time (Outliers Removed)')
    plt.legend()
    plt.grid(True)

    # Save the plot to a file
    plt.savefig('calibrated_torque_filtered_over_time.png')

    # Display the plot
    plt.show()



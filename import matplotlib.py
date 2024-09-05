import pandas as pd
import matplotlib.pyplot as plt

# Function to process the CSV data
def process_data(file_path):
    # Read the CSV file into a DataFrame
    data = pd.read_csv(file_path)
    
    # Skip to every 10th row for processing
    processed_data = data.iloc[::10, :]

    # Print processed data for verification
    print("Processed Data:")
    print(processed_data)

    # Extracting the values for plotting
    time = processed_data['Time']
    angular_speed = processed_data['AngularSpeed']
    
    # Create placeholders for magnitudes and frequencies (not printed now)
    magnitudes = []
    frequencies = []
    for i in range(1, 5):  # Looping from 1 to 4 for intervals
        # Normally you would parse these values from some source
        magnitude = 1.00  # hardcoded example value
        frequency = 0.24 - (i * 0.02)  # Adjust to reflect example frequency trend
        magnitudes.append(f'Magnitude for Interval {i}: {magnitude:.2f}')
        frequencies.append(f'Frequency for Interval {i}: {frequency:.2f}')

    # Printing Magnitude and Frequency sections is removed.
    # The graph will still be created.

    # Plotting the data
    plt.figure(figsize=(10, 5))
    plt.plot(time, angular_speed, marker='o', linestyle='-', color='b')
    plt.title('Angular Speed Over Time')
    plt.xlabel('Time')
    plt.ylabel('Angular Speed')
    plt.grid()
    plt.tight_layout()  # Adjust layout to make room for titles and labels
    plt.savefig('angular_speed_plot.png')  # Save plot as PNG
    plt.show()

# File path for the CSV file
file_path = 'Data.csv'  # Make sure the file is in the same directory or provide an absolute path.

# Call the function to process data
process_data(file_path)
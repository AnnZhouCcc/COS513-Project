import numpy as np
import pandas as pd

seed = 1
np.random.seed(seed)

data_randomseed = 8
data_cca_arr = [1,6]

output_data_path_prefix = "/u/az6922/COS513-Project/preprocess/data/"

num_ports = 40
train_ratio = 0.8 # train:test = 8:2
sequence_length = 1000
step_length = 10

for data_cca in data_cca_arr:
    train_data_path = output_data_path_prefix + "X"+str(data_cca)+"_train.csv"
    target_data_path = output_data_path_prefix + "y"+str(data_cca)+"_train.csv"
    test_data_path = output_data_path_prefix + "X"+str(data_cca)+"_test.csv"

    print("loading data")
    input_data_path = "/u/az6922/data/randomseed"+str(data_randomseed)+"/tor-single-"+str(data_cca)+"-101-0.9-0.9-3.stat"
    df = pd.read_csv(input_data_path, delimiter=' ',nrows=20000).iloc[:,4:]

    print("parsing data")
    headers = []
    for i in range(num_ports):
        headers.append("qsize_"+str(i)+"_0")
        headers.append("qsize_"+str(i)+"_1")
    df.columns = headers

    df_switch0 = df.iloc[::2,:]
    df_switch1 = df.iloc[1::2,:]
    df_switch0.reset_index(drop=True,inplace=True)
    df_switch1.reset_index(drop=True,inplace=True)

    num_rows = df_switch0.shape[0]
    train_size = int(num_rows*train_ratio)
    test_size = num_rows - train_size
    df_switch0_train = df_switch0.iloc[:train_size,:]
    df_switch1_train = df_switch1.iloc[:train_size,:]
    df_switch0_test = df_switch0.iloc[train_size:,:]
    df_switch1_test = df_switch1.iloc[train_size:,:]

    print("Generating training data")
    df_train_list = []
    num_pieces = int((train_size - sequence_length)/step_length)+1
    for i in range(num_pieces):
        piece_start = 0+i*step_length
        piece_end = piece_start + sequence_length
        piece = df_switch0_train.iloc[piece_start:piece_end,:]
        piece['series_id'] = i
        piece['timestamp'] = np.arange(sequence_length)
        df_train_list.append(piece)
    for i in range(num_pieces):
        piece_start = 0+i*step_length
        piece_end = piece_start + sequence_length
        piece = df_switch1_train.iloc[piece_start:piece_end,:]
        piece['series_id'] = i+num_pieces
        piece['timestamp'] = np.arange(sequence_length)
        df_train_list.append(piece)
    data_x_train = pd.concat(df_train_list, ignore_index=True)
    #print(data_x_train.shape)
    #print(data_x_train.iloc[699995:700005,77:])

    data_y_train = pd.DataFrame()
    data_y_train['series_id'] = np.arange(num_pieces*2)
    if data_cca == 1:
        data_y_train['cca_id'] = 0 # CUBIC
    elif data_cca == 6:
        data_y_train['cca_id'] = 1 # Timely
    else:
        print("data_cca = " + str(data_cca) + " is not recognized.")

    print("Generating testing data")
    df_test_list = []
    num_test_pieces = int((test_size - sequence_length)/step_length)+1
    for i in range(num_test_pieces):
        piece_start = 0+i*step_length
        piece_end = piece_start + sequence_length
        piece = df_switch0_test.iloc[piece_start:piece_end,:]
        piece['series_id'] = i
        piece['timestamp'] = np.arange(sequence_length)
        df_test_list.append(piece)
    for i in range(num_test_pieces):
        piece_start = 0+i*step_length
        piece_end = piece_start + sequence_length
        piece = df_switch1_test.iloc[piece_start:piece_end,:]
        piece['series_id'] = i+num_test_pieces
        piece['timestamp'] = np.arange(sequence_length)
        df_test_list.append(piece)
    data_x_test = pd.concat(df_test_list, ignore_index=True)
    #print(data_x_test.shape)

    print("Writing data")
    data_x_train.to_csv(train_data_path, index=False)
    data_y_train.to_csv(target_data_path, index=False)
    data_x_test.to_csv(test_data_path, index=False)

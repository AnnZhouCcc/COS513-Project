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

df_train_list = []
num_train_pieces = 0
df_test_list = []
num_test_pieces = 0
piece_index = 0
test_piece_index = 0
for data_cca_index, data_cca in enumerate(data_cca_arr):
    print(f'cca {data_cca}')
    train_data_path = output_data_path_prefix + "X_perqueue_train.csv"
    target_data_path = output_data_path_prefix + "y_perqueue_train.csv"
    test_data_path = output_data_path_prefix + "X_perqueue_test.csv"
    ytest_data_path = output_data_path_prefix + "y_perqueue_test.csv"

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
    num_pieces = int((train_size - sequence_length)/step_length)+1
    num_train_pieces = num_pieces
    for q in range(num_ports*2):
        for i in range(num_pieces):
            piece_start = 0+i*step_length
            piece_end = piece_start + sequence_length
            piece = df_switch0_train.iloc[piece_start:piece_end,q].to_frame()
            piece.columns = ['qsize']
            piece['series_id'] = piece_index
            piece['timestamp'] = np.arange(sequence_length)
            df_train_list.append(piece)
            piece_index += 1

    for q in range(num_ports*2):
        for i in range(num_pieces):
            piece_start = 0+i*step_length
            piece_end = piece_start + sequence_length
            piece = df_switch1_train.iloc[piece_start:piece_end,q].to_frame()
            piece.columns = ['qsize']
            piece['series_id'] = piece_index
            piece['timestamp'] = np.arange(sequence_length)
            df_train_list.append(piece)
            piece_index += 1

    print("Generating testing data")
    num_test_pieces = int((test_size - sequence_length)/step_length)+1
    for q in range(num_ports*2):
        for i in range(num_test_pieces):
            piece_start = 0+i*step_length
            piece_end = piece_start + sequence_length
            piece = df_switch0_test.iloc[piece_start:piece_end,q].to_frame()
            piece.columns = ['qsize']
            piece['series_id'] = test_piece_index
            piece['timestamp'] = np.arange(sequence_length)
            df_test_list.append(piece)
            test_piece_index += 1

    for q in range(num_ports*2):
        for i in range(num_test_pieces):
            piece_start = 0+i*step_length
            piece_end = piece_start + sequence_length
            piece = df_switch1_test.iloc[piece_start:piece_end,q].to_frame()
            piece.columns = ['qsize']
            piece['series_id'] = test_piece_index
            piece['timestamp'] = np.arange(sequence_length)
            df_test_list.append(piece)
            test_piece_index += 1

print("Writing data")
data_x_train = pd.concat(df_train_list, ignore_index=True)
print(data_x_train.shape)
data_x_train.to_csv(train_data_path, index=False)

data_x_test = pd.concat(df_test_list, ignore_index=True)
print(data_x_test.shape)
data_x_test.to_csv(test_data_path, index=False)

total_train_len = len(data_cca_arr)*num_train_pieces*2*num_ports*2
data_y_train = pd.DataFrame()
data_y_train['series_id'] = np.arange(total_train_len)
data_y_train['cca_id'] = 0 # CUBIC
data_y_train.iloc[int(total_train_len/2):,1] = 1 # Timely
print(data_y_train.shape)
print(num_train_pieces)
print(total_train_len)
print(piece_index)
#print(data_y_train.iloc[1400:1410,:])
data_y_train.to_csv(target_data_path, index=False)

total_test_len = len(data_cca_arr)*num_test_pieces*2*num_ports*2
data_y_test = pd.DataFrame()
data_y_test['series_id'] = np.arange(total_test_len)
data_y_test['cca_id'] = 0 # CUBIC
data_y_test.iloc[int(total_test_len/2):,1] = 1 # Timely
print(data_y_test.shape)
print(num_test_pieces)
print(total_test_len)
print(test_piece_index)
data_y_test.to_csv(ytest_data_path, index=False)

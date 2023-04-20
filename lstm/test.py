import numpy as np
import pandas as pd
import torch
from torch import nn
from torch.nn import functional as F
from torch.utils.data import DataLoader,TensorDataset
from sklearn.preprocessing import LabelEncoder
from LSTMClassifier import LSTMClassifier

ID_COLS = ['series_id','timestamp']

which_cca_train = 6 # 1 for CUBIC, 6 for Timely
which_cca_test = 6

num_ports = 40

data_path_prefix = "/u/az6922/COS513-Project/preprocess/data/"
test_data_path = data_path_prefix+"X"+str(which_cca_test)+"_test.csv"
target_data_path = data_path_prefix+"y"+str(which_cca_train)+"_train.csv"


x_cols = {
    'series_id':np.int32,
    'timestamp':np.int32
}
for i in range(num_ports):
    key = 'qsize_'+str(i)+'_0'
    x_cols[key] = np.float32
    key = 'qsize_'+str(i)+'_1'
    x_cols[key] = np.float32

y_cols = {
    'series_id':np.int32,
    'cca_id':np.int32
}

x_test_raw = pd.read_csv(test_data_path, usecols=x_cols.keys(),dtype=x_cols)
y_train_raw = pd.read_csv(target_data_path, usecols=y_cols.keys(),dtype=y_cols)

def create_test_dataset(X, drop_cols=ID_COLS):
    X_grouped = np.row_stack([
        group.drop(columns=drop_cols).values[None]
        for _, group in X.groupby('series_id')])
    X_grouped = torch.tensor(X_grouped.transpose(0, 2, 1)).float()
    y_fake = torch.tensor([0] * len(X_grouped)).long()
    return TensorDataset(X_grouped, y_fake)

input_dim = 80
hidden_dim = 256
layer_dim = 3
output_dim = 2

model = LSTMClassifier(input_dim, hidden_dim, layer_dim, output_dim)
model.load_state_dict(torch.load('best_'+str(which_cca_train)+'.pth'))
model.eval()

batch_size = 80
test_dl = DataLoader(create_test_dataset(x_test_raw), batch_size, shuffle=False)

test = []
prob = []
print('Predicting on test dataset')
for batch, _ in test_dl:
    batch = batch.permute(0, 2, 1)
    #out = model(batch.cuda())
    out = model(batch)
    y_hat = F.log_softmax(out, dim=1).argmax(dim=1)
    test += y_hat.tolist()
    prob += F.log_softmax(out, dim=1).tolist()

enc = LabelEncoder()
enc.fit_transform(y_train_raw['cca_id'])
print(prob)
print(test)
print(enc.inverse_transform(test))

import numpy as np
import pandas as pd
import torch
from torch import nn
from torch.nn import functional as F
from torch.utils.data import DataLoader,TensorDataset
from sklearn.preprocessing import LabelEncoder
from LSTMClassifier import LSTMClassifier

seed = 1
np.random.seed(seed)
torch.cuda.set_device(0)

ID_COLS = ['series_id','timestamp']

hidden_dim = 256
layer_dim = 2

num_ports = 40

#data_path_prefix = "/u/az6922/COS513-Project/preprocess/data/"
data_path_prefix = "/home/ruipan/buffer/COS513-Project/preprocess/data/"
test_data_path = data_path_prefix+"X_perqueue_test.csv"
ytest_data_path = data_path_prefix+"y_perqueue_test.csv"
target_data_path = data_path_prefix+"y_perqueue_train.csv"


x_cols = {
    'qsize':np.float32,
    'series_id':np.int32,
    'timestamp':np.int32
}

y_cols = {
    'series_id':np.int32,
    'cca_id':np.int32
}

x_test_raw = pd.read_csv(test_data_path, usecols=x_cols.keys(),dtype=x_cols)
y_test_raw = pd.read_csv(ytest_data_path, usecols=y_cols.keys(),dtype=y_cols)
y_train_raw = pd.read_csv(target_data_path, usecols=y_cols.keys(),dtype=y_cols)

def create_test_dataset(X, drop_cols=ID_COLS):
    X_grouped = np.row_stack([
        group.drop(columns=drop_cols).values[None]
        for _, group in X.groupby('series_id')])
    #X_grouped = torch.tensor(X_grouped.transpose(0, 2, 1)).float()
    X_grouped = torch.tensor(X_grouped).float()
    y_fake = torch.tensor([0] * len(X_grouped)).long()
    return TensorDataset(X_grouped, y_fake)

input_dim = 1
output_dim = 2

model = LSTMClassifier(input_dim, hidden_dim, layer_dim, output_dim)
model = model.cuda()
model.load_state_dict(torch.load('best_perqueue_l'+str(layer_dim)+'_d'+str(hidden_dim)+'.pth'))
model.eval()

batch_size = 512
test_dl = DataLoader(create_test_dataset(x_test_raw), batch_size, shuffle=False)

test = []
print('Predicting on test dataset')
count = 0
for batch, _ in test_dl:
    count += 1
    #batch = batch.permute(0, 2, 1)
    out = model(batch.cuda())
    #out = model(batch)
    y_hat = F.log_softmax(out, dim=1).argmax(dim=1)
    test += y_hat.tolist()

print(count)
enc = LabelEncoder()
enc.fit_transform(y_train_raw['cca_id'])
print(enc.classes_)
#print(test)
#print(enc.inverse_transform(test))
truth = y_test_raw['cca_id'].tolist()
total = 0
correct = 0
assert len(truth) == len(test)
for i in range(len(truth)):
    total += 1
    if truth[i] == test[i]:
        correct += 1
print(f'total={total}, correct={correct}, correct rate={correct/total}')

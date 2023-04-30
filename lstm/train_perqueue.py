import numpy as np
import pandas as pd
from sklearn.preprocessing import LabelEncoder
from sklearn.model_selection import train_test_split
import torch
from torch import nn
from torch.nn import functional as F
from torch.utils.data import TensorDataset,DataLoader
from torch.optim.lr_scheduler import _LRScheduler
from multiprocessing import cpu_count
from LSTMClassifier import LSTMClassifier

num_ports = 40

data_path_prefix = "/u/az6922/COS513-Project/preprocess/data/"
train_data_path = data_path_prefix + "X_perqueue_train.csv"
target_data_path = data_path_prefix+"y_perqueue_train.csv"

seed = 1
np.random.seed(seed)
#torch.cuda.set_device(0)

ID_COLS = ['series_id','timestamp']

print("Reading data")
x_cols = {
    'qsize':np.float32,
    'series_id':np.int32,
    'timestamp':np.int32
}

y_cols = {
    'series_id':np.int32,
    'cca_id':np.int32
}

x_train_raw = pd.read_csv(train_data_path, usecols=x_cols.keys(),dtype=x_cols)
y_train_raw = pd.read_csv(target_data_path, usecols=y_cols.keys(),dtype=y_cols)

print(x_train_raw.shape)
print(y_train_raw.shape)

def create_datasets(X,y):
    valid_size = 0.25
    drop_cols = ID_COLS
    
    enc = LabelEncoder()
    y_enc = enc.fit_transform(y)
    X_grouped = create_grouped_X(X)
    #X_grouped = X_grouped.transpose(0,2,1)
    X_train, X_valid, y_train, y_valid = train_test_split(X_grouped, y_enc, test_size=valid_size)
    X_train, X_valid = [torch.tensor(arr, dtype=torch.float32) for arr in (X_train, X_valid)]
    y_train, y_valid = [torch.tensor(arr, dtype=torch.long) for arr in (y_train, y_valid)]
    train_ds = TensorDataset(X_train, y_train)
    valid_ds = TensorDataset(X_valid, y_valid)
    return train_ds, valid_ds, enc

def create_grouped_X(X):
    group_col = 'series_id'
    drop_cols = ID_COLS

    X_grouped = np.row_stack([
        group.drop(columns=drop_cols).values[None]
        for _,group in X.groupby(group_col)])
    print(X_grouped.shape)
    return X_grouped

def create_loaders(train_dataset, valid_dataset, batch_size, jobs):
    train_dl = DataLoader(train_dataset, batch_size, shuffle=True, num_workers=jobs)
    valid_dl = DataLoader(valid_dataset, batch_size, shuffle=False, num_workers=jobs)
    return train_dl, valid_dl


#class LSTMClassifier(nn.Module):
#    def __init__(self,input_dim,hidden_dim,layer_dim,output_dim):
#        super().__init__()
#        self.hidden_dim = hidden_dim
#        self.layer_dim = layer_dim
#        self.rnn = nn.LSTM(input_dim, hidden_dim, layer_dim, batch_first=True)
#        self.fc = nn.Linear(hidden_dim, output_dim)
#        self.batch_size = None
#        self.hidden = None
#
#    def forward(self, x):
#        h0, c0 = self.init_hidden(x)
#        out, (hn, cn) = self.rnn(x, (h0, c0))
#        out = self.fc(out[:, -1, :])
#        return out
#
#    def init_hidden(self, x):
#        h0 = torch.zeros(self.layer_dim, x.size(0), self.hidden_dim)
#        c0 = torch.zeros(self.layer_dim, x.size(0), self.hidden_dim)
#        #return [t.cuda() for t in (h0, c0)]
#        return [t for t in (h0, c0)]

class CyclicLR(_LRScheduler):
    def __init__(self, optimizer, schedule, last_epoch=-1):
        assert callable(schedule)
        self.schedule = schedule
        super().__init__(optimizer, last_epoch)

    def get_lr(self):
        return [self.schedule(self.last_epoch, lr) for lr in self.base_lrs]

def cosine(t_max, eta_min=0):
    def scheduler(epoch, base_lr):
        t = epoch % t_max
        return eta_min + (base_lr - eta_min)*(1 + np.cos(np.pi*t/t_max))/2

    return scheduler

print("Preparing datasets")
train_dataset, valid_dataset, encoder = create_datasets(x_train_raw,y_train_raw['cca_id'])

batch_size = 4096
print("Creating data loaders")
train_dl, valid_dl = create_loaders(train_dataset,valid_dataset,batch_size,jobs=cpu_count())

input_dim = 1
hidden_dim = 256
layer_dim = 2
output_dim = 2
seq_dim = 1000

lr = 0.0005
n_epochs = 100
iterations_per_epoch = len(train_dl)
best_accuracy = 0
patience = 100
trials = 0

model = LSTMClassifier(input_dim, hidden_dim, layer_dim, output_dim)
#model = model.cuda()
criterion = nn.CrossEntropyLoss()
#criterion = nn.BCELoss()
opt = torch.optim.RMSprop(model.parameters(), lr=lr)
sched = CyclicLR(opt, cosine(t_max=iterations_per_epoch * 2, eta_min=lr/100))

print('Start model training')

for epoch in range(n_epochs):
    for i, (x_batch, y_batch) in enumerate(train_dl):
        model.train()
        #x_batch = x_batch.cuda()
        #y_batch = y_batch.cuda()
        #sched.step()
        opt.zero_grad()
        out = model(x_batch)
        loss = criterion(out, y_batch)
        loss.backward()
        opt.step()
        sched.step()

    model.eval()
    correct, total = 0, 0
    for x_val, y_val in valid_dl:
        #x_val, y_val = [t.cuda() for t in (x_val, y_val)]
        x_val, y_val = [t for t in (x_val, y_val)]
        out = model(x_val)
        preds = F.log_softmax(out, dim=1).argmax(dim=1)
        total += y_val.size(0)
        correct += (preds == y_val).sum().item()

    acc = correct / total

    #if epoch % 5 == 0 or epoch == n_epochs-1:
    print(f'Epoch: {epoch:3d}. Loss: {loss.item():.4f}. Acc.: {acc:2.2%}')

    if acc > best_accuracy:
        trials = 0
        best_accuracy = acc
        torch.save(model.state_dict(), 'best_perqueue_l'+str(layer_dim)+'_d'+str(hidden_dim)+'.pth')
        print(f'Epoch {epoch} best model saved with accuracy: {best_accuracy:2.2%}')
    else:
        trials += 1
        if trials >= patience:
            print(f'Early stopping on epoch {epoch}')
            break

    if best_accuracy == 1:
        print(f'Early stopping on epoch {epoch}')
        break

print('The training is finished! Restoring the best model weights')

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

class RawDataVisualization:
    def __init__(self,num_ports,randomseed_arr,cca_arr) -> None:
        self.num_ports = num_ports
        self.randomseed_arr = randomseed_arr
        self.cca_arr = cca_arr

    def load_parse_plot(self):
        for randomseed in self.randomseed_arr:
            for cca in self.cca_arr:

                print("loading data")
                data_file = "/u/az6922/data/randomseed"+str(randomseed)+"/tor-single-"+str(cca)+"-101-0.9-0.9-3.stat"
                df = pd.read_csv(data_file, delimiter=' ',nrows=20000).iloc[:,4:]
                
                print("parsing data")
                part1 = df.iloc[:,::2]
                part2 = df.iloc[:,1::2]
                headers = []
                for i in range(self.num_ports):
                    headers.append("port"+str(i))
                part1.columns = headers
                part2.columns = headers
                df_stacked = pd.concat([part1, part2], ignore_index=True)

                df0 = df_stacked.iloc[::2,:]
                df0.reset_index(drop=True,inplace=True)
                df1 = df_stacked.iloc[1::2,:]
                df1.reset_index(drop=True,inplace=True)

                df0_nonzero = df0.T.loc[(df0.T!=0).any(axis=1)].T
                df1_nonzero = df1.T.loc[(df1.T!=0).any(axis=1)].T

                print(df0_nonzero.shape)
                print(df1_nonzero.shape)

                print("plotting")
                df0_nonzero["time"] = np.arange(df0_nonzero.shape[0])
                print(df0_nonzero.shape)
                df1_nonzero["time"] = np.arange(df1_nonzero.shape[0])
                print(df1_nonzero.shape)

                for switch_index in range(2):
                    print(switch_index)
                    num_columns = 0
                    if switch_index == 0:
                        num_columns = df0_nonzero.shape[1]
                        port_names = list(df0_nonzero.columns)[:-1]
                    elif switch_index == 1:
                        num_columns = df1_nonzero.shape[1]
                        port_names = list(df1_nonzero.columns)[:-1]

                    if switch_index == 0:
                        df0_nonzero.plot(x="time",y=port_names,kind="line",figsize=(10,10))
                        plt.savefig("randomseed"+str(randomseed)+"_cca"+str(cca)+"_switch0.pdf")
                    elif switch_index == 1:
                        df1_nonzero.plot(x="time",y=port_names,kind="line",figsize=(10,10))
                        plt.savefig("randomseed"+str(randomseed)+"_cca"+str(cca)+"_switch1.pdf")


if __name__ == "__main__":
    num_ports = 40
    randomseed_arr = [5,8]
    cca_arr = [1,4,6]
    RawDataVisualization(num_ports,randomseed_arr,cca_arr).load_parse_plot()

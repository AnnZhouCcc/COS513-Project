# print the configurations of trials that result in all bursty flows not slowed down when sharing with continuous flows
def trials_all_zero_fct_slowdown(dir, starttime_list, initialwindow_list, numcontinous, numbursty):
	for iw in initialwindow_list:
		for start in starttime_list:
			fctslowdown_allseedslist = list()
			for seed in range(1,11):
				# Read fct from file
				cwbfile = dir+"star-burst-tolerance-flow-"+str(iw)+"-"+str(start)+"-0-"+str(seed)+".data"
				bonlyfile = dir+"star-burst-tolerance-flow-"+str(iw)+"-"+str(start)+"-1-"+str(seed)+".data"
				cwb_fct = dict()
				bonly_fct = dict()
				with open(cwbfile) as cwbf:
					count = 0
					for line in cwbf:
						count += 1
						if count <= 1: continue
						array = [x for x in line.split()]
						flowsize = float(array[1])
						fct = float(array[2])
						cwb_fct[flowsize] = fct
				with open(bonlyfile) as bonlyf:
					count = 0
					for line in bonlyf:
						count += 1
						if count <= numcontinous+1: continue
						array = [x for x in line.split()]
						flowsize = float(array[1])
						fct = float(array[2])
						bonly_fct[flowsize] = fct
				
				# Calculate fct slowdown
				fctslowdown = list()
				assert(len(cwb_fct) == numbursty)
				assert(len(bonly_fct) == numbursty)
				for cwb_flow_size, cwb_flow_fct in cwb_fct.items():
					for bonly_flow_size, bonly_flow_fct in bonly_fct.items():
						if cwb_flow_size == bonly_flow_size:
							fctslowdown.append(cwb_flow_fct/bonly_flow_fct)
							break
				fctslowdown_allseedslist.append(fctslowdown)
				#print(str(iw)+" "+str(start))
				#print(fctslowdown)
				#print(cwb_fct)
				#print(bonly_fct)

			# Inspect which trials have all flows with 0 FCT slowdown
			for index,slowdownlist in enumerate(fctslowdown_allseedslist):
				should_print = True
				for sd in slowdownlist:
					if sd != 1:
						should_print = False
						break
				if should_print:
					print(f"iw={iw},start={start},seed={index}")

	return

if __name__ == "__main__":
	dir = "/u/az6922/data/burst-tolerance-aug5/"
	start_list = [0,4500]
	iw_list = [5,50,100,500,1000]
	numcontinuous = 10
	numbursty = 10
	trials_all_zero_fct_slowdown(dir, start_list, iw_list, numcontinuous, numbursty)

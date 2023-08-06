def summarize_flow(dir, starttime_list, initialwindow_list):
	outfile1 = dir + "average_all.txt"
	outfile2 = dir + "average_slow.txt"
	outfile3 = dir + "num_slow.txt"
	open(outfile1, 'w').close()
	open(outfile2, 'w').close()
	open(outfile3, 'w').close()
	for iw in initialwindow_list:
		for start in starttime_list:
			fctslowdown_allseedslist = list()
			for seed in range(1,11):
				# Read fct from file
				cwbfile = dir+"star-burst-tolerance-flow-"+str(iw)+"-"+str(start)+"-0-"+str(seed)+".data"
				bonlyfile = dir+"star-burst-tolerance-flow-"+str(iw)+"-"+str(start)+"-1-"+str(seed)+".data"
				cwb_fct = list()
				bonly_fct = list()
				with open(cwbfile) as cwbf:
					count = 0
					for line in cwbf:
						count += 1
						if count <= 1: continue
						array = [x for x in line.split()]
						cwb_fct.append(array[2])
				with open(bonlyfile) as bonlyf:
					count = 0
					for line in bonlyf:
						count += 1
						if count <= 11: continue
						array = [x for x in line.split()]
						bonly_fct.append(array[2])
				
				# Calculate fct slowdown
				fctslowdown = list()
				for i in range(10):
					fctslowdown.append(float(cwb_fct[i])/float(bonly_fct[i]))
				fctslowdown_allseedslist.append(fctslowdown)
				#print(str(iw)+" "+str(start))
				#print(fctslowdown)
				#print(cwb_fct)
				#print(bonly_fct)

			# Write out tables
			# 1. Average FCT slowdown across all flows
			sum_fct_all = 0
			count_fct_all = 0
			for slowdownlist in fctslowdown_allseedslist:
				avg_sd = sum(slowdownlist) / len(slowdownlist)
				sum_fct_all += avg_sd
				count_fct_all += 1
			f1 = open(outfile1, "a")
			f1.write(str(sum_fct_all/count_fct_all)+"\t")
			f1.close()

			# 2. Average FCT slowdown across flows that have been slowed down
			sum_fct_slow = 0
			count_fct_slow = 0
			for slowdownlist in fctslowdown_allseedslist:
				sum_sd = 0
				count_sd = 0
				for sd in slowdownlist:
					if sd > 1:
						sum_sd += sd
						count_sd += 1
				if count_sd > 0:
					avg_sd = sum_sd / count_sd
				else:
					avg_sd = 0
				if avg_sd > 0:
					sum_fct_slow += avg_sd
					count_fct_slow += 1
			f2 = open(outfile2, "a")
			f2.write(str(sum_fct_slow/count_fct_slow)+"\t")
			f2.close()

			# 3. Number of flows that have been slowed down
			count_num_slow = 0
			for slowdownlist in fctslowdown_allseedslist:
				count_sd = 0
				for sd in slowdownlist:
					if sd > 1:
						count_sd += 1
				count_num_slow += count_sd
			f3 = open(outfile3, "a")
			f3.write(str(count_num_slow/10.0)+"\t")
			f3.close()

		f1 = open(outfile1, "a")
		f1.write("\n")
		f1.close()
		f2 = open(outfile2, "a")
		f2.write("\n")
		f2.close()
		f3 = open(outfile3, "a")
		f3.write("\n")
		f3.close()

	return

if __name__ == "__main__":
	dir = "/u/az6922/data/burst-tolerance-aug5/"
	start_list = [0,4500]
	iw_list = [5,50,100,500,1000]
	summarize_flow(dir, start_list, iw_list)

"""A "fork" of plot.py to make it work with TwoChoice_v2.

"""

import numpy as np, pandas, time
import matplotlib.pyplot as plt
import my
import scipy.stats
import trials_info_tools # replace this with specifics
import TrialSpeak, TrialMatrix

o2c = {'hit': 'g', 'error': 'r', 'spoil': 'k', 'curr': 'white'}


def format_perf_string(nhit, ntot):
    """Helper function for turning hits and totals into a fraction."""
    perf = nhit / float(ntot) if ntot > 0 else 0.
    res = '%d/%d=%0.2f' % (nhit, ntot, perf)
    return res





class Plotter(object):
    """Base class for plotters by stim number or servo throw.
    
    Child classes MUST define the following:
    assign_trial_type_to_trials_info
    get_list_of_trial_type_names
    """
    def __init__(self, trial_plot_window_size=50):
        """Initialize base Plotter class."""
        # Size of trial window
        self.trial_plot_window_size = trial_plot_window_size
        
        # Anova caching
        self.cached_anova_text1 = ''
        self.cached_anova_len1 = 0
        self.cached_anova_text2 = ''
        self.cached_anova_len2 = 0       
    
    def init_handles(self):
        """Create graphics handles"""
        # Plot 
        f, ax = plt.subplots(1, 1, figsize=(11, 4))
        f.subplots_adjust(left=.35, right=.95, top=.75)
        
        # Make handles to each outcome
        label2lines = {}
        for outcome, color in o2c.items():
            label2lines[outcome], = ax.plot(
                [None], [None], 'o', label=outcome, color=color)
        
        # Plot the bads
        label2lines['bad'], = ax.plot(
            [None], [None], '|', label='bad', color='k', ms=10)

        # Plot a horizontal line at SERVO_THROW if available
        label2lines['divis'], = ax.plot(
            [None], [None], 'k-', label='divis')

        # Separate y-axis for rewards
        ax2 = ax.twinx()
        ax2.set_ylabel('rewards')

        # Store graphics handles
        self.graphics_handles = {
            'f': f, 'ax': ax, 'ax2': ax2, 'label2lines': label2lines}
        
        # create the window
        plt.show()
    
    def update_trial_type_parameters(self, lines):
        """Update parameters relating to trial type.
        
        Does nothing by default but child classes will redefine."""
        pass
    
    def update(self, filename):   
        """Read info from filename and update the plot"""
        ## Load data and make trials_info
        # Check log
        lines = TrialSpeak.read_lines_from_file(filename)
        splines = TrialSpeak.split_by_trial(lines)        
        
        # Really we should wait until we hear something from the arduino
        # Simply wait till at least one line has been received
        if len(splines) == 0 or len(splines[0]) == 0:
            return

        # Construct trial_matrix. I believe this will always have at least
        # one line in it now, even if it's composed entirely of Nones.
        trials_info = TrialMatrix.make_trials_info_from_splines(splines)

        ## Translate condensed trialspeak into full data
        # Put this part into TrialSpeak.py
        trials_info = TrialSpeak.translate_trial_matrix(trials_info)
        
        # fake this for now
        trials_info['bad'] = False 


        ## Define trial types, the ordering on the plot
        # Make any updates to trial type parameters (child-class-dependent)
        self.update_trial_type_parameters(lines)
        
        # Add type information to trials_info and generate type names
        trials_info = self.assign_trial_type_to_trials_info(trials_info)
        trial_type_names = self.get_list_of_trial_type_names()

        
        ## Count performance by type
        # Hits by type
        typ2perf = trials_info_tools.count_hits_by_type_from_trials_info(
            trials_info[~trials_info.bad])
        typ2perf_all = trials_info_tools.count_hits_by_type_from_trials_info(
            trials_info)
        
        # Hits by side
        side2perf = trials_info_tools.count_hits_by_type_from_trials_info(
            trials_info[~trials_info.bad], split_key='rewside')
        side2perf_all = trials_info_tools.count_hits_by_type_from_trials_info(
            trials_info, split_key='rewside')            
        
        # Combined
        total_nhit, total_ntot = trials_info_tools.calculate_nhit_ntot(
            trials_info[~trials_info.bad])

        # Turn the typ2perf into ticklabels
        ytick_labels = typ2perf2ytick_labels(trial_type_names, 
            typ2perf, typ2perf_all)


        ## count rewards
        # Get the rewards by each trial in splines
        n_rewards_l = []
        for nspline, spline in enumerate(splines):
            n_rewards = np.sum(map(lambda s: 'EVENT REWARD' in s, spline))
            n_rewards_l.append(n_rewards)
        n_rewards_a = np.asarray(n_rewards_l)
        
        # Match those onto the rewards from each side
        l_rewards = np.sum(n_rewards_a[(trials_info['rewside'] == 0).values])
        r_rewards = np.sum(n_rewards_a[(trials_info['rewside'] == 1).values])
        
        # turn the rewards into a title string
        title_string = '%d rewards L; %d rewards R;\n' % (l_rewards, r_rewards)
        
        anova_stats = ''
        ## A line of info about unforced trials
        title_string += 'UF: '
        if 0 in side2perf:
            title_string += 'L: ' + \
                format_perf_string(side2perf[0][0], side2perf[0][1]) + '; '
        if 1 in side2perf:
            title_string += 'R: ' + \
                format_perf_string(side2perf[1][0], side2perf[1][1]) + ';'
        #~ if len(trials_info) > self.cached_anova_len1 or self.cached_anova_text1 == '':
            #~ anova_stats = trials_info_tools.run_anova(
                #~ trials_info, remove_bad=True)
            #~ self.cached_anova_text1 = anova_stats
            #~ self.cached_anova_len1 = len(trials_info)
        #~ else:
            #~ anova_stats = self.cached_anova_text1
        title_string += '. Biases: ' + anova_stats
        title_string += '\n'
        
        
        ## A line of info about all trials
        title_string += 'All: '
        if 0 in side2perf_all:
            title_string += 'L_A: ' + \
                format_perf_string(side2perf_all[0][0], side2perf_all[0][1]) + '; '
        if 1 in side2perf_all:
            title_string += 'R_A: ' + \
                format_perf_string(side2perf_all[1][0], side2perf_all[1][1])
        #~ if len(trials_info) > self.cached_anova_len2 + 5 or self.cached_anova_text2 == '':
            #~ # Need to numericate before anova
            #~ ## This is needed for anova, not sure where it belongs
            #~ trials_info['prevchoice'] = trials_info['choice'].shift(1)
            #~ trials_info['prevchoice'][trials_info.prevchoice.isnull()] = -1
            #~ trials_info['prevchoice'] = trials_info['prevchoice'].astype(np.int)                    
            
            #~ anova_stats = trials_info_tools.run_anova(
                #~ trials_info, remove_bad=False)
            #~ self.cached_anova_text2 = anova_stats
            #~ self.cached_anova_len2 = len(trials_info)
        #~ else:
            #~ anova_stats = self.cached_anova_text2
        title_string += '. Biases: ' + anova_stats
        
        ## PLOTTING REWARDS
        # Plot the rewards as a separate trace
        for line in self.graphics_handles['ax2'].lines:
            line.remove()    
        self.graphics_handles['ax2'].plot(
            np.arange(len(n_rewards_a)), n_rewards_a, 'k-')
        self.graphics_handles['ax2'].set_yticks(
            np.arange(np.max(n_rewards_a) + 2))


        ## PLOTTING
        # plot each outcome
        for outcome in ['hit', 'error', 'spoil', 'curr']:
            # Get rows corresponding to this outcome
            msk = trials_info['outcome'] == outcome

            # Get the line corresponding to this outcome and set the xdata
            # to the appropriate trial numbers and the ydata to the trial types
            line = self.graphics_handles['label2lines'][outcome]
            line.set_xdata(np.where(msk)[0])
            line.set_ydata(trials_info['trial_type'][msk])

        # plot vert bars where bad trials occurred
        msk = trials_info['bad']
        line = self.graphics_handles['label2lines']['bad']
        line.set_xdata(np.where(msk)[0])
        line.set_ydata(trials_info['trial_type'][msk])


        ## PLOTTING axis labels and title
        ax = self.graphics_handles['ax']
        
        # Use the ytick_labels calculated above
        ax.set_yticks(range(len(trial_type_names)))
        ax.set_yticklabels(ytick_labels, size='small')
        
        # The ylimits go BACKWARDS so that trial types are from top to bottom
        ymax = trials_info['trial_type'].max()
        ymin = trials_info['trial_type'].min()
        ax.set_ylim((ymax + .5, ymin -.5))
        
        # The xlimits are a sliding window of size TRIAL_PLOT_WINDOW_SIZE
        ax.set_xlim((
            len(trials_info) - self.trial_plot_window_size, 
            len(trials_info)))    
        
        # title set above
        ax.set_title(title_string, size='medium')
        
        ## plot division between L and R
        line = self.graphics_handles['label2lines']['divis']
        #~ line.set_xdata(ax.get_xlim())
        #~ line.set_ydata([self.servo_throw - .5] * 2)
        
        ## PLOTTING finalize
        plt.show()
        plt.draw()    
    
    def update_till_interrupt(self, filename, interval=.3):
        # update over and over
        PROFILE_MODE = False

        try:
            while True:
                self.update(filename)
                
                if not PROFILE_MODE:
                    time.sleep(interval)
                else:
                    break

        except KeyboardInterrupt:
            plt.close('all')
            print "Done."
        except:
            raise
        finally:
            pass



class PlotterByStimNumber(Plotter):
    """Plots performance by stim number."""
    def __init__(self, n_stimuli=6, **base_kwargs):
        # Initialize base
        super(PlotterByStimNumber, self).__init__(**base_kwargs)
        
        # Initialize me
        # This could be inferred
        self.n_stimuli = n_stimuli

    def update_trial_type_parameters(self, lines):
        """Looks for changes in number of stimuli.
        
        lines: read from file
        """
        pass
    
    def assign_trial_type_to_trials_info(self, trials_info):
        """Returns a copy of trials_info with a column called trial_type.
        
        Trial type is defined by this object as stim number.
        """
        trials_info = trials_info.copy()
        trials_info['trial_type'] = trials_info['stim_number']
        return trials_info

    def get_list_of_trial_type_names(self):
        """Return the name of each trial type.
        
        This object assumes left and right stimuli are alternating and that
        the very first trial_type is a left stimulus.
        
        Returns ['LEFT 0', 'RIGHT 1', 'LEFT 2', ...] up to self.n_stimuli
        """
        res = []
        for sn in range(self.n_stimuli):
            # All even are LEFT
            side = 'LEFT' if np.mod(sn, 2) == 0 else 'RIGHT'
            res.append(side + ' %d' % sn)        
        return res


class PlotterWithServoThrow(Plotter):
    """Object encapsulating the logic and parameters to plot trials by throw."""
    def __init__(self, trial_types, **base_kwargs):
        # Initialize base
        super(PlotterWithServoThrow, self).__init__(**base_kwargs)
        
        # Initialize me
        self.trial_types = trial_types
        
    def assign_trial_type_to_trials_info(self, trials_info):
        """Returns a copy of trials_info with a column called trial_type.
        
        We match the srvpos and stppos variables in trials_info to the 
        corresponding rows of self.trial_types. The index of the matching row
        is the trial type for that trial.
        
        Warnings are issued if keywords are missing, multiple matches are 
        found (in which case the first is used), or no match is found
        (in which case the first trial type is used, although this should
        probably be changed to None).
        """
        trials_info = trials_info.copy()
        
        # Set up the pick kwargs for how we're going to pick the matching type
        # The key is the name in self.trial_types, and the value is the name
        # in trials_info
        pick_kwargs = {'stppos': 'stepper_pos', 'srvpos': 'servo_pos', 
            'rewside': 'rewside'}
        
        # Test for missing kwargs
        warn_missing_kwarg = []
        for key, val in pick_kwargs.items():
            if val not in trials_info.columns:
                pick_kwargs.pop(key)
                warn_missing_kwarg.append(key)
        if len(warn_missing_kwarg) > 0:
            print "warning: missing kwargs to match trial type:" + \
                ' '.join(warn_missing_kwarg)
        
        # Iterate over trials
        # Could probably be done more efficiently with a groupby
        trial_types_l = []
        warn_no_matches = []
        warn_multiple_matches = []
        warn_missing_data = []
        warn_type_error = []
        for idx, ti_row in trials_info.iterrows():
            # Pick the matching row in trial_types
            trial_pick_kwargs = dict([
                (k, ti_row[v]) for k, v in pick_kwargs.items() 
                if not pandas.isnull(ti_row[v])])
            
            # Try to pick
            try:
                pick_idxs = my.pick(self.trial_types, **trial_pick_kwargs)
            except TypeError:
                # typically, comparing string with int
                warn_type_error.append(idx)
                pick_idxs = [0]
            
            # error check missing data
            if len(trial_pick_kwargs) < len(pick_kwargs):
                warn_missing_data.append(idx)            
            
            # error-check and reduce to single index
            if len(pick_idxs) == 0:
                # no match, use the first trial type
                1/0
                warn_no_matches.append(idx)
                pick_idx = 0
            elif len(pick_idxs) > 1:
                # multiple match
                warn_multiple_matches.append(idx)
                pick_idx = pick_idxs[0]
            else:
                # no error
                pick_idx = pick_idxs[0]
            
            # Store result
            trial_types_l.append(pick_idx)

        # issue warnings
        if len(warn_type_error) > 0:
            print "error: type error in pick on trials " + \
                ' '.join(map(str, warn_type_error))
        if len(warn_missing_data) > 0:
            print "error: missing data on trials " + \
                ' '.join(map(str, warn_missing_data))
        if len(warn_no_matches) > 0:
            print "error: no matches found in some trials " + \
                ' '.join(map(str, warn_no_matches))
        elif len(warn_multiple_matches) > 0:
            print "error: multiple matches found on some trials"

        # Put into trials_info and return
        trials_info['trial_type'] = trial_types_l
        return trials_info

    def get_list_of_trial_type_names(self):
        """Name of each trial type."""
        res = list(self.trial_types['name'])        
        return res


## This is all for the updating by time, instead of trial
def update_by_time_till_interrupt(plotter, filename):
    # update over and over
    PROFILE_MODE = False

    try:
        while True:
            # This part differs between the by trial and by time functions
            # Update_by_time should be updating the data, not replotting
            for line in plotter['ax'].lines:
                line.remove()
            
            update_by_time(plotter, filename)
            
            if not PROFILE_MODE:
                time.sleep(.3)
            else:
                break

    except KeyboardInterrupt:
        plt.close('all')
        print "Done."
    except:
        raise
    finally:
        pass



def init_by_time(**kwargs):
    # Plot 
    f, ax = plt.subplots(figsize=(10, 2))
    f.subplots_adjust(left=.2, right=.95, top=.85)

    # create the window
    plt.show()

    return {'f': f, 'ax': ax}

def update_by_time(plotter, filename):
    ax = plotter['ax']
    #~ label2lines = plotter['label2lines']    
    
    with file(filename) as fi:
        lines = fi.readlines()

    #rew_lines = filter(lambda line: line.startswith('REWARD'), lines)
    rew_lines_l = filter(lambda line: 'EVENT REWARD_L' in line, lines)
    rew_times_l = np.array(map(lambda line: int(line.split()[0])/1000., 
        rew_lines_l))
    rew_lines_r = filter(lambda line: 'EVENT REWARD_R' in line, lines)
    rew_times_r = np.array(map(lambda line: int(line.split()[0])/1000., 
        rew_lines_r))

    if len(rew_times_l) + len(rew_times_r) == 0:
        counts_l, edges = [0], [0, 1]
        counts_r, edges = [0], [0, 1]
    else:
        binlen = 20
        bins = np.arange(0, 
            np.max(np.concatenate([rew_times_l, rew_times_r])) + 2* binlen, 
            binlen)
        counts_l, edges = np.histogram(rew_times_l, bins=bins)
        counts_r, edges = np.histogram(rew_times_r, bins=bins)

    ax.plot(edges[:-1], counts_l, 'b')
    ax.plot(edges[:-1], counts_r, 'r')
    ax.set_xlabel('time (s)')
    ax.set_ylabel('rewards')
    ax.set_title('%d %d rewards' % tuple(map(len, [rew_times_l, rew_times_r])))
    plt.draw()
    plt.show()    



## Utility functions
def typ2perf2ytick_labels(trial_type_names, typ2perf, typ2perf_all):
    """Go through types and make ytick label about the perf for each."""
    ytick_labels = []
    for typnum, typname in enumerate(trial_type_names):
        tick_label = typname + ':'
        
        if typnum in typ2perf:
            nhits, ntots = typ2perf[typnum]
            tick_label += ' Unforced:%03d/%03d' % (nhits, ntots)
            if ntots > 0:
                tick_label += '=%0.2f' % (float(nhits) / ntots)
            tick_label += '.'
        
        if typnum in typ2perf_all:
            nhits, ntots = typ2perf_all[typnum]
            tick_label += ' All:%03d/%03d' % (nhits, ntots)
            if ntots > 0:
                tick_label += '=%0.2f' % (float(nhits) / ntots)
            tick_label += '.'        
    
        ytick_labels.append(tick_label)
    
    return ytick_labels
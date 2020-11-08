//Andy Meng
//Console Based studco election counter
//known Issues:
//  program may break if emails or candidate names have comma inside them

//#define DEBUG

#include <iostream>
#include <vector>
#include <map>
#include <fstream>
using namespace std;

struct stats //structure to store voter statistics
{
    map<int, int> population; //map of the total lisgar population read from the email list. the key is the grade, and value is # of students for that grade
    map<int, int> validVotes; //same as before, but read from the poll stats. value is the number of valid votes for that grade
    map<int, int> invalidVotes; //same as before, but the value is the number of invalid votes
};

struct ballot //structure to store each ballot
{
    pair<string, int> studentInfo; //first element is the email, second is the grade
    int vote; //index of which candidate they voted for
    string invalidReason; //a string to say why their vote was invalid if this is an invalid vote
};

struct candidate //structure for each candidate
{
    //names are all pretty self explanatory
    vector<ballot> validVotes;
    vector<ballot> invalidVotes;
    string name;
    int grade;
};

bool populateMaster(map<string, int> &m, stats &s, string fileName) //populate email list
{
    ifstream fin (fileName); //opens email list and adds all values into the map
    while(!fin.eof())
    {
        string email;
        int grade;
        fin >> email >> grade;
        if (!email.compare(""))
        {
            break;
        }
        m.insert(pair<string, int>(email, grade));
    }

    //iterating through each email to error check
    map<string, int>::iterator it;
    for(it = m.begin(); it != m.end(); it++ )
    {
        if(it->first.length() >= 9 && it->first.substr(it->first.length() - 9, 9).compare("@ocdsb.ca")!= 0) //isolate the last 9 characters of an email to determine whether or not it ends in @ocdsb.ca
        {
            cout << "masterlist read-in error: email detected not ending in @ocdsb.ca." << endl;
            cout << "Error email: " << it->first << endl;
            return false;
        }
        if(it->second < 8 || it->second > 13) //check the grade of the student. using wider values to ensure that only random values are caught. (does the school officially store a student as grade 13?)
        {
            cout << "masterlist read-in error: grade detected smaller than 8 or larger than 13" << endl;
            cout << "Error email: " << it->first << ", associated grade: " << it->second << endl;
            return false;
        }

        if (s.population.find(it->second) != s.population.end()) //check if a grade exists in the population map
        {
            s.population[it->second]++; //increment that population's grade by 1 if it already exists
        }
        else
        {
            s.population[it->second] = 1; //set that grade's population to 1 if it's the first entry for that grade
        }
    }
    return true;
}

bool populateResults(map<string, int> m, vector<ballot> &r, vector<candidate> &t, string fileName, int electionGrade)
{
    ifstream fin(fileName);

    string buffer;
    getline(fin, buffer); //getline to remove the title from the file buffer
    while(!fin.eof())
    {
        getline(fin, buffer); //parse the entire entry into a string

        int letterIterator = 0; //iterate through each character

        //strings to store the 3 separate points of data
        string timestamp = "";
        string candidateName = "";
        string email = "";

        while(buffer[letterIterator] != ',')//loop until first comma is reached, adding each character into the timestamp
        {
            timestamp += buffer[letterIterator];
            letterIterator++;
        }

        letterIterator++; //iterate past first comma and add each character into the candidate name
        while(buffer[letterIterator] != ',')
        {
            candidateName += buffer[letterIterator];
            letterIterator++;
        }

        letterIterator++; //loop until the end of the string, adding each character to email string
        while(letterIterator < buffer.length())
        {
            email += buffer[letterIterator];
            letterIterator++;
        }

        ballot currentBallot;//create ballot instance to store information
        currentBallot.studentInfo.first = email;//set email
        currentBallot.studentInfo.second = 0;//set default grade to 0, only populate if the information is found from masterlist

        currentBallot.vote = -1; //set default vote index to -1
        for(int i = 0; i < t.size(); i++)//change to non-negative value if the corresponding candidate is found
        {
            if(candidateName.compare(t[i].name) == 0)
            {
                currentBallot.vote = i;
            }
        }

        if(currentBallot.vote == -1)//if no candidate is found with the same name, this entry must be the first entry for a given candidate, so add them into the candidates vector
        {
            candidate newCandidate;
            newCandidate.grade = electionGrade;
            newCandidate.name = candidateName;
            t.push_back(newCandidate);
            currentBallot.vote = t.size()-1; //adjust the ballot so it votes towards the last candidate since it was most recently added
        }

        if(email.length() >= 9 && email.substr(email.length() - 9, 9).compare("@ocdsb.ca")!= 0)//check if email is an ocdsb email
        {
            #ifdef DEBUG
            cout << email << " is not ocdsb email" << endl;
            #endif
        }
        else if(m.find(email) != m.end()) //check if email can be found in database.
        {
            currentBallot.studentInfo.second = m[email]; //email was found, add grade information to current ballot from master list
        }
        else //email is an ocdsb email but not a lisgar email
        {
            #ifdef DEBUG
            cout << email << " is not lisgar email" << endl;
            #endif
        }
        #ifdef DEBUG
        cout << timestamp << "/" << candidateName << "/" << email << endl;
        #endif

        r.push_back(currentBallot); //add ballot to list of all parsed ballots
    }
    return true;
}

//takes all parsed ballots and assigns them to their corresponding candidates
bool tallyVotes(vector<candidate> &t, vector<ballot> r, map<string, int> m, stats &s)
{
    //iterates through each ballot
    for(int i = 0; i < r.size(); i++)
    {
        ballot currentBallot = r[i]; //store current ballot to make it easier to refer to
        if(m.find(currentBallot.studentInfo.first) != m.end() && m[currentBallot.studentInfo.first] == t[currentBallot.vote].grade) //if email is found inside list and grade matches
        {
            t[currentBallot.vote].validVotes.push_back(currentBallot); //add ballot to their corresponding candidate's list of valid votes
            if (s.validVotes.find(currentBallot.studentInfo.second) != s.population.end()) //add one to the valid vote counter
            {
                s.validVotes[currentBallot.studentInfo.second]++;
            }
            else
            {
                s.validVotes[currentBallot.studentInfo.second] = 1;
            }
        }
        else
        {
            t[currentBallot.vote].invalidVotes.push_back(currentBallot);//add current vote to list of invalid votes

            if(m.find(currentBallot.studentInfo.first) == m.end())//find reason why vote is invalid and store in string
            {
                t[currentBallot.vote].invalidVotes[t[currentBallot.vote].invalidVotes.size()-1].invalidReason = "vote registered from an email not associated with a lisgar student";
            }
            else if (currentBallot.studentInfo.second != t[currentBallot.vote].grade)
            {
                t[currentBallot.vote].invalidVotes[t[currentBallot.vote].invalidVotes.size()-1].invalidReason = "student voter grade and candidate grade mismatch";
            }

            if (s.invalidVotes.find(currentBallot.studentInfo.second) != s.population.end())//add one to invalid vote counter
            {
                s.invalidVotes[currentBallot.studentInfo.second]++;
            }
            else
            {
                s.invalidVotes[currentBallot.studentInfo.second] = 1;
            }
        }
    }
    return true;
}

bool outputTally(vector<candidate> t, string electionGrade, stats &s)
{
    ofstream summary ("output/summary" + electionGrade + ".txt"); //create summary file
    summary << "Election Summary for the grade " << electionGrade << " election:" << endl;

    //loop through all candidates
    for(int i = 0; i < t.size(); i++)
    {

        //add current candidate information to summary page
        summary << "Candidate " << t[i].name << " received " << t[i].validVotes.size() << " valid votes" << endl;
        summary << "\t This is " << float(t[i].validVotes.size())/s.population[t[i].grade]*100 << "% of the total grade " << t[i].grade << " population at lisgar." << endl;
        summary << "\t This is " << float(t[i].validVotes.size())/s.validVotes[t[i].grade]*100 << "% of the total voting grade " << t[i].grade << " population at lisgar." << endl << endl;

        //write detailed information about each candidate into their respective outputs
        string fileName = "output/" + t[i].name + ".txt";
        ofstream fout (fileName);
        fout << "Grade " << t[i].grade << " Candidate " << t[i].name << "(index " << i << "): ";
        fout << t[i].validVotes.size() << " valid vote(s) and ";
        fout << t[i].invalidVotes.size() << " invalid vote(s)." << endl;
        fout << "See below for each vote" << endl << endl;


        fout << "-----VALID VOTES-----" << endl;
        for(int j = 0; j < t[i].validVotes.size(); j++)
        {
            fout << "\t";
            fout << "email: " << t[i].validVotes[j].studentInfo.first << ", grade: " << t[i].validVotes[j].studentInfo.second << ", candidate index: " << t[i].validVotes[j].vote << endl;
        }

        fout << "-----INVALID VOTES-----" << endl;
        for(int j = 0; j < t[i].invalidVotes.size(); j++)
        {
            fout << "\t";
            fout << "email: " << t[i].invalidVotes[j].studentInfo.first << ", grade: " << t[i].invalidVotes[j].studentInfo.second << ", candidate index: " << t[i].invalidVotes[j].vote << ", Invalid vote reason: " << t[i].invalidVotes[j].invalidReason << endl;
        }
        fout.close();
    }

}

int main()
{
    string buffer; //temporary trash buffer
    string masterFileName; //name of email list
    string electionGrade; //grade of current election being tallied
    string csvFileName; //csv for poll results
    ifstream settings ("settings.txt"); //settings file

    //parse settings
    getline(settings, buffer);
    getline(settings, masterFileName);
    getline(settings, buffer);
    getline(settings, buffer);
    getline(settings, electionGrade);
    getline(settings, buffer);
    getline(settings, buffer);
    getline(settings, csvFileName);

    map<string, int> masterlist; //masterlist of emails and grades
    vector<ballot> pollResults; //parsed poll data including email, grade, and which person they voted for
    vector<candidate> tally; //number of votes for each candidate
    stats voterStats;//statistics on voter population

    if (!populateMaster(masterlist, voterStats, masterFileName)) //exit if there is an issue parsing email list
    {
        return 1;
    }

    if (!populateResults(masterlist, pollResults, tally, csvFileName, stoi(electionGrade))) //exit if there is an issue parsing poll results
    {
        return 1;
    }


    tallyVotes(tally, pollResults, masterlist, voterStats); //tally all votes

    outputTally(tally, electionGrade, voterStats); //output the tally

    #ifdef DEBUG
    map<string, int>::iterator it;
    for(it = masterlist.begin(); it != masterlist.end(); it++ )
    {
        cout << it->first << " " << it->second << endl;
    }
    #endif

    return 0;
}

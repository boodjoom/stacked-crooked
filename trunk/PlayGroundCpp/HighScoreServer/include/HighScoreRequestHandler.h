#ifndef HIGHSCOREREQUESTHANDLER_H_INCLUDED
#define HIGHSCOREREQUESTHANDLER_H_INCLUDED


#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Data/Session.h"
#include <map>
#include <string>


namespace HSServer
{

    typedef std::map<std::string, std::string> Args;


    class HighScoreRequestHandler : public Poco::Net::HTTPRequestHandler
    {
    public:
        HighScoreRequestHandler();

        void handleRequest(Poco::Net::HTTPServerRequest& inRequest,
                           Poco::Net::HTTPServerResponse& inResponse);

        
        static void GetArgs(const std::string & inURI, Args & outArgs);

    protected:
        virtual void generateResponse(Poco::Data::Session & inSession, std::ostream & ostr) = 0;

    private:
        Poco::Data::Session mSession;
    };


    class HighScore
    {
    public:
        HighScore(const std::string & inName, int inScore) : mName(inName), mScore(inScore) {}

        const std::string & name() const { return mName; }

        int score() const { return mScore; }

    private:
        std::string mName;
        int mScore;
    };

        
    class DefaultRequestHandler : public HighScoreRequestHandler
    {
    public:        
        static DefaultRequestHandler * Create(const std::string & inURI);

    protected:
        virtual void generateResponse(Poco::Data::Session & inSession, std::ostream & ostr);

    private:
        DefaultRequestHandler() {}
    };

        
    class GetAllHighScores : public HighScoreRequestHandler
    {
    public:        
        static GetAllHighScores * Create(const std::string & inURI);

    protected:
        virtual void generateResponse(Poco::Data::Session & inSession, std::ostream & ostr);

    private:
        GetAllHighScores() {}
    };


    class AddHighScore : public HighScoreRequestHandler
    {
    public:
        static AddHighScore * Create(const std::string & inURI);

    protected:
        virtual void generateResponse(Poco::Data::Session & inSession, std::ostream & ostr);

    private:
        AddHighScore(const HighScore & inHighScore);
        HighScore mHighScore;
    };

} // HighScoreServer


#endif // HIGHSCOREREQUESTHANDLER_H_INCLUDED

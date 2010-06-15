#include "HighScoreRequestHandler.h"
#include "FileUtils.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Data/SessionFactory.h"
#include "Poco/Data/SQLite/Connector.h"
#include "Poco/Util/Application.h"
#include "Poco/StreamCopier.h"
#include "Poco/String.h"
#include "Poco/StringTokenizer.h"
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <iostream>
#include <sstream>


using namespace Poco::Data;


namespace HSServer
{

    MissingArgumentException::MissingArgumentException(const std::string & inMessage) :
        std::runtime_error(inMessage)
    {
    }

    
    HighScoreRequestHandler::HighScoreRequestHandler() :
        mSession(SessionFactory::instance().create("SQLite", "HighScores.db"))
    {
        // Create the table if it doesn't already exist
        mSession << "CREATE TABLE IF NOT EXISTS HighScores(Id INTEGER PRIMARY KEY, Name VARCHAR(20), Score INTEGER(5))", now;
    }


    void HighScoreRequestHandler::GetArgs(const std::string & inURI, Args & outArgs)
    {
        if (inURI.empty())
        {
            return;
        }

        if (inURI[0] != '/')
        {
            return;
        }

        std::string argString = inURI.substr(1, inURI.size() - 1);
        Poco::StringTokenizer tokenizer(argString, "&", Poco::StringTokenizer::TOK_IGNORE_EMPTY |
                                                        Poco::StringTokenizer::TOK_TRIM);

        Poco::StringTokenizer::Iterator it = tokenizer.begin(), end = tokenizer.end();
        for (; it != end; ++it)
        {
            const std::string & pair = *it;
            Poco::StringTokenizer t(pair, "=", Poco::StringTokenizer::TOK_IGNORE_EMPTY |
                                               Poco::StringTokenizer::TOK_TRIM);
            if (t.count() != 2)
            {
                continue;
            }
            outArgs.insert(std::make_pair(t[0], t[1]));
        }
    }


    const std::string & HighScoreRequestHandler::GetArg(const Args & inArgs, const std::string & inArg)
    {
        Args::const_iterator it = inArgs.find(inArg);
        if (it != inArgs.end())
        {
            return it->second;
        }
        throw MissingArgumentException("Missing argument: " + inArg);
    }


    std::string HighScoreRequestHandler::getContentType() const
    {
        return "text/html";
    }

    
    void HighScoreRequestHandler::handleRequest(Poco::Net::HTTPServerRequest& inRequest,
                                                Poco::Net::HTTPServerResponse& inResponse)
    {
        Poco::Util::Application& app = Poco::Util::Application::instance();
        app.logger().information("Request from " + inRequest.clientAddress().toString());

        inResponse.setChunkedTransferEncoding(true);
        inResponse.setContentType(getContentType());

        std::ostream & outStream = inResponse.send();
        try
        {
            generateResponse(mSession, outStream);
        }
        catch (const std::exception & inException)
        {
            app.logger().critical(inException.what());
        }
    }    
    
    
    HighScoreRequestHandler * DefaultRequestHandler::Create(const std::string & inURI)
    {
        return new DefaultRequestHandler;
    }


    void DefaultRequestHandler::generateResponse(Poco::Data::Session & inSession, std::ostream & ostr)
    {
        std::ifstream html("html/index.html");
        Poco::StreamCopier::copyStream(html, ostr);
    }


    ErrorRequestHandler::ErrorRequestHandler(const std::string & inErrorMessage) :
        mErrorMessage(inErrorMessage)
    {
    }


    void ErrorRequestHandler::generateResponse(Poco::Data::Session & inSession, std::ostream & ostr)
    {
        ostr << "<html><body>";
        ostr << "<h1>Error</h1>";
        ostr << "<p>" + mErrorMessage + "</p>";
        ostr << "</body></html>";
    }
    
    
    HighScoreRequestHandler * GetAllHighScores::Create(const std::string & inURI)
    {
        return new GetAllHighScores;
    }


    std::string GetStringValue(const Poco::DynamicAny & inDynamicValue, const std::string & inDefault)
    {
        if (inDynamicValue.isString())
        {
            return static_cast<std::string>(inDynamicValue);
        }
        else if (inDynamicValue.isNumeric())
        {
            return boost::lexical_cast<std::string>(static_cast<int>(inDynamicValue));
        }
        return inDefault;
    }


    void GetAllHighScores::getRows(const Poco::Data::RecordSet & inRecordSet, std::string & outRows)
    {
        for (size_t rowIdx = 0; rowIdx != inRecordSet.rowCount(); ++rowIdx)
        {   
            outRows += "<tr>";
            for (size_t colIdx = 0; colIdx != inRecordSet.columnCount(); ++colIdx)
            {                
                outRows += "<td>";
                outRows += GetStringValue(inRecordSet.value(colIdx, rowIdx), "(Unknown DynamicAny)");
                outRows += "</td>";
            }
            outRows += "</tr>\n";
        }
    }


    void GetAllHighScores::generateResponse(Poco::Data::Session & inSession, std::ostream & ostr)
    {   
        Statement select(inSession);
        select << "SELECT * FROM HighScores";
        select.execute();
        RecordSet rs(select);

        std::string html;
        HSServer::ReadEntireFile("html/getall.html", html);
        std::string rows;
        getRows(rs, rows);
        ostr << Poco::replace<std::string>(html, "{{ROWS}}", rows);
    }
    
    
    HighScoreRequestHandler * AddHighScore::Create(const std::string & inURI)
    {
        return new AddHighScore;
    }


    AddHighScore::AddHighScore()
    {
    }


    void AddHighScore::generateResponse(Poco::Data::Session & inSession, std::ostream & ostr)
    {
        std::ifstream htmlFile("html/add.html");
        Poco::StreamCopier::copyStream(htmlFile, ostr);
    }
    
    
    HighScoreRequestHandler * CommitHighScore::Create(const std::string & inURI)
    {
        Args args;
        GetArgs(inURI, args);
        return new CommitHighScore(GetArg(args, "name"), GetArg(args, "score"));
    }


    CommitHighScore::CommitHighScore(const std::string & inName, const std::string & inScore) :
        mName(inName),
        mScore(inScore)
    {
    }


    void CommitHighScore::generateResponse(Poco::Data::Session & inSession, std::ostream & ostr)
    {        
        Statement insert(inSession);        
        insert << "INSERT INTO HighScores VALUES(NULL, ?, ?)", use(mName),
                                                               use(mScore);
        insert.execute();

        // Return an URL instead of a HTML page.
        // This is because the client is the JavaScript application in this case.
        ostr << "http://localhost/hs/commit-succeeded&name=" << mName << "&score=" << mScore;
    }


    std::string CommitHighScore::getContentType() const
    {
        // Return value is simply an URL.
        return "text/plain";
    }
    
    
    HighScoreRequestHandler * CommitSucceeded::Create(const std::string & inURI)
    {
        Args args;
        GetArgs(inURI, args);
        return new CommitSucceeded(GetArg(args, "name"), GetArg(args, "score"));
    }


    CommitSucceeded::CommitSucceeded(const std::string & inName, const std::string & inScore) :
        mName(inName),
        mScore(inScore)
    {
    }


    void CommitSucceeded::generateResponse(Poco::Data::Session & inSession, std::ostream & ostr)
    {
        ostr << "<html>";
        ostr << "<body>";
        ostr << "<p>";
        ostr << "Succesfully added highscore for " << mName << " of " << mScore << ".";
        ostr << "</p>";
        ostr << "</body>";
        ostr << "</html>";
    }

} // namespace HSServer
